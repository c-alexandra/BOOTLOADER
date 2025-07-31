/*******************************************************************************
 * @file   bootloader.c
 * @author Camille Alexandra
 *
 * @brief  Redirect vector table to launch with custom bootloader
 ******************************************************************************/

#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/stm32/rcc.h>
// #include <libopencm3/cm3/vector.h> // if using libopencm3 vector table
#include <libopencm3/cm3/scb.h> // system control block

// User includes
#include "common.h"
#include "core/uart.h"
#include "core/system.h"
#include "core/gpio.h"
#include "comms.h"
#include "bl-flash.h"
#include "core/simple-timer.h"
#include "core/shift-register.h"
#include "core/firmware-info.h"

// Arbitrary sync sequence used to identify the start of a firmware update
#define SYNC_SEQUENCE_0 (0xC4)
#define SYNC_SEQUENCE_1 (0x55)
#define SYNC_SEQUENCE_2 (0x7E)
#define SYNC_SEQUENCE_3 (0x10)

#define DEFAULT_TIMEOUT (5000)  // default timeout at 5s
#define SHORT_TIMEOUT   (1000)  // short timeout at 1s
#define LONG_TIMEOUT    (15000) // long timeout at 15s

// bootloader state machine states
typedef enum bl_state_t {
    BL_STATE_SYNC,
    BL_STATE_UPDATE_REQ,
    BL_STATE_DEVICE_ID_REQ,
    BL_STATE_DEVICE_ID_RESP,
    BL_STATE_FW_LENGTH_REQ,
    BL_STATE_FW_LENGTH_RESP,
    BL_STATE_APPLICATION_ERASE,
    BL_STATE_RECEIVE_FW,
    BL_STATE_DONE,
} bl_state_t;

static bl_state_t bl_state = BL_STATE_SYNC;
static uint32_t fw_length = 0; // length of firmware to be received in bytes
static uint32_t fw_bytes_written = 0; // track bytes written to flash
static uint8_t sync_seq[4] = {0};
static simple_timer_t timer; // module-level timer we will use for timeouts
static comms_packet_t packet; 

ShiftRegister8_t sr1 = {
        .led_state = 0x00,
        .num_outputs = 8, // 8 outputs for the debug LEDs
        .gpio_port = SR1_PORT,
        .ser_pin = SR1_DATA_PIN,
        .srclk_pin = SR1_CLOCK_PIN,
        .rclk_pin = SR1_LATCH_PIN
    };

/*******************************************************************************
 * @brief Targets the main application start address and jumps to it using
 *        reset vector.
 ******************************************************************************/
static void jump_to_main(void) {
    // create typedef for a void function
    typedef void (*void_fn)(void);

    // reset vector is second entry in table, stack pointer is first, thus + 4
    uint32_t* reset_vector_entry = (uint32_t*)(MAIN_APP_START_ADDRESS + 4U);
    uint32_t* reset_vector = (uint32_t*)(*reset_vector_entry);

    // interpret reset_vector as void function and call function
    void_fn reset_fnc = (void_fn)reset_vector;

    reset_fnc();

    // The following implementation uses the libopencm3 vector table
    // and uses the libopencm3/cm3/vector.h file to call the reset function
    // vector_table_t* vector_table = (vector_table_t*)MAIN_APP_START_ADDRESS;
    // main_vector_table->reset();
}

/*******************************************************************************
 * @brief Debug function which breaks linker script if rom size set to 32kb
 ******************************************************************************/
// const uint8_t data[0x8000] = {0};
// static void test_rom_size(void) {
//     volatile uint8_t x = 0;
//     for (uint32_t i = 0; i < 0x8000; ++i) {
//         x += data[i];
//     }
// }

/*******************************************************************************
 * @brief Setup GPIO pins for UART communication
 ******************************************************************************/
static void gpio_setup(void) {
    // enable rcc for GPIOA
    rcc_periph_clock_enable(RCC_GPIOA);

    //configure pins for uart
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN | RX_PIN);
    gpio_set_af(UART_PORT, GPIO_AF7, TX_PIN | RX_PIN);
}

/*******************************************************************************
 * @brief Teardown for GPIO before jumping to main app
 ******************************************************************************/
static void gpio_teardown(void) {
    gpio_mode_setup(UART_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
        TX_PIN | RX_PIN); // set uart pins to analog mode
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
        GPIO4); // set LED pin to input
    rcc_periph_clock_disable(RCC_GPIOA);
}

/*******************************************************************************
 * @brief Sends NACK packet over SERIAL connection and begins jump to main app
 ******************************************************************************/
static void abort_fw_update(void) {
    comms_create_single_byte_packet(&packet, BL_PACKET_NACK_DATA0);
    comms_send_packet(&packet);
    bl_state = BL_STATE_DONE;
}

/*******************************************************************************
 * @brief Checks bootloader update timeout and aborts if it has expired
 ******************************************************************************/
static void check_update_timeout(void) {
    if (simple_timer_check_has_expired(&timer)) {
        abort_fw_update();
    }
}

/*******************************************************************************
 * @brief Check if a given packet matches signature of device id packet
 *
 * @param packet Pointer to the packet to check
 * @return True if the packet is a device id packet, False otherwise
 * 
 * @note A device id packet will have a length of 2 bytes, with the first
 *       byte being BL_PACKET_DEVICE_ID_RESPONSE_DATA0, and the following
 *       byte being the device id
 ******************************************************************************/
static bool is_device_id_packet(const comms_packet_t* verify_packet) {
    if (verify_packet->length != 2) {
        return false;
    }

    if (verify_packet->data[0] != BL_PACKET_DEVICE_ID_RESPONSE_DATA0) {
        return false;
    }

    for (uint8_t i = 2; i < PACKET_DATA_LENGTH; ++i) {
        if (verify_packet->data[i] != 0xFF) {
            return false;
        }
    }
    
    return true;
}

/*******************************************************************************
 * @brief Check if a given packet matches signature of firmware length packet
 * 
 * @param packet Pointer to the packet to check
 * @return True if the packet is a firmware length packet, False otherwise
 * 
 * @note A firmware length packet will have a length of 5 bytes, with the first
 *       byte being BL_PACKET_FW_LENGTH_RESPONSE_DATA0, and the following 4
 *       corresponsing to a uint32_t firmware length
 ******************************************************************************/
static bool is_fw_length_packet(const comms_packet_t* verify_packet) {
    if (verify_packet->length != 5) {
        return false;
    }

    if (verify_packet->data[0] != BL_PACKET_FW_LENGTH_RESPONSE_DATA0) {
        return false;
    }

    for (uint8_t i = 5; i < PACKET_DATA_LENGTH; ++i) {
        if (verify_packet->data[i] != 0xFF) {
            return false;
        }
    }
    
    return true;
}

int main(void) {
    // initialize system peripherals
    system_setup();
    gpio_setup();
    uart_setup();
    comms_setup();
    shift_register_setup(&sr1);

    // initialize module level timer to check fw update timeouts
    simple_timer_setup(&timer, DEFAULT_TIMEOUT, false);

    while (1) {
        // TODO: change implementation to utilize packet protocol and state 
        // machine for all states
        
        // utilizes raw serial bytes, so handle separately from other states
        if (bl_state == BL_STATE_SYNC) {

            shift_register_set_pattern(&sr1, SR_DEBUG_1); // TODO

            if (uart_data_available()) {
                sync_seq[0] = sync_seq[1];
                sync_seq[1] = sync_seq[2];
                sync_seq[2] = sync_seq[3];
                sync_seq[3] = uart_receive_byte();

                bool sequence_match = sync_seq[0] == SYNC_SEQUENCE_0;
                sequence_match &= sync_seq[1] == SYNC_SEQUENCE_1;
                sequence_match &= sync_seq[2] == SYNC_SEQUENCE_2;
                sequence_match &= sync_seq[3] == SYNC_SEQUENCE_3;

                if (sequence_match) {
                    comms_create_single_byte_packet(&packet, 
                        BL_PACKET_SYNC_OBSERVED_DATA0);
                    comms_send_packet(&packet);
                    simple_timer_reset(&timer);
                    bl_state = BL_STATE_UPDATE_REQ;
                } else { // check for timeout on sync
                    check_update_timeout();
                }
            } else { // check for timeout on sync 
                check_update_timeout();
            }
            continue;
        }

        comms_update(); // begin using packet protocol to receive packets

        /* handle bootloader state machine
         * Communicates back and forth between the bootloader and device via 
         * UART packets, initiating a firmware update process if available 
         */
        switch (bl_state) {

            case BL_STATE_UPDATE_REQ: {
                shift_register_set_pattern(&sr1, SR_DEBUG_2);

                if (comms_data_available()) {
                    comms_receive_packet(&packet);
                    
                    if (comms_is_single_byte_packet(&packet, BL_PACKET_FW_UPDATE_REQUEST_DATA0)) {
                        comms_create_single_byte_packet(&packet, BL_PACKET_FW_UPDATE_RESPONSE_DATA0);
                        comms_send_packet(&packet);
                        simple_timer_reset(&timer);
                        bl_state = BL_STATE_DEVICE_ID_REQ;
                    } else {
                        abort_fw_update();
                    }
                } else {
                    check_update_timeout();
                }
            } break;

            case BL_STATE_DEVICE_ID_REQ: {
                shift_register_set_pattern(&sr1, SR_DEBUG_3);


                simple_timer_reset(&timer);
                comms_create_single_byte_packet(&packet, 
                    BL_PACKET_DEVICE_ID_REQUEST_DATA0);
                comms_send_packet(&packet);
                bl_state = BL_STATE_DEVICE_ID_RESP;
            } break;

            case BL_STATE_DEVICE_ID_RESP: {
                shift_register_set_pattern(&sr1, SR_DEBUG_4);

                if (comms_data_available()) {
                    comms_receive_packet(&packet);
                    
                    if (is_device_id_packet(&packet)) {
                        if (packet.data[1] == DEVICE_ID) {
                            simple_timer_reset(&timer);
                            bl_state = BL_STATE_FW_LENGTH_REQ;
                        } else {
                            abort_fw_update();
                        }
                    } 
                } else {
                    check_update_timeout();
                }
            } break;

            case BL_STATE_FW_LENGTH_REQ: {
                shift_register_set_pattern(&sr1, SR_DEBUG_5);
                comms_create_single_byte_packet(&packet,
                     BL_PACKET_FW_LENGTH_REQUEST_DATA0);
                comms_send_packet(&packet);
                simple_timer_reset(&timer);
                bl_state = BL_STATE_FW_LENGTH_RESP;
            } break;

            case BL_STATE_FW_LENGTH_RESP: {
                shift_register_set_pattern(&sr1, SR_DEBUG_6);
                if (comms_data_available()) {
                    comms_receive_packet(&packet);

                    fw_length = *((uint32_t*)&packet.data[1]);
                    // fw_length = (
                    //     (packet.data[1])      |
                    //     (packet.data[2]) << 8 |
                    //     (packet.data[3]) << 16|
                    //     (packet.data[4]) << 24
                    // );
                    
                    if (is_fw_length_packet(&packet) 
                    && fw_length <= MAX_FW_LENGTH) {
                        simple_timer_reset(&timer);
                        bl_state = BL_STATE_APPLICATION_ERASE;
                    } else {
                        abort_fw_update();
                    }
                } else {
                    check_update_timeout();
                }
            } break;

            case BL_STATE_APPLICATION_ERASE: {
                shift_register_set_pattern(&sr1, SR_DEBUG_7);
                bl_flash_erase_main_app(); // can take ~10s

                // send ready for data packet whenever we want to receive data
                // blocking operation, takes time
                comms_create_single_byte_packet(&packet, 
                    BL_PACKET_READY_FOR_DATA_DATA0);
                comms_send_packet(&packet);

                simple_timer_reset(&timer);
                bl_state = BL_STATE_RECEIVE_FW;
            } break;

            case BL_STATE_RECEIVE_FW: {
                shift_register_set_pattern(&sr1, SR_DEBUG_8);
                if (comms_data_available()) {
                    comms_receive_packet(&packet);
                    
                    // write packet data to flash memory
                    bl_flash_write_main_app(MAIN_APP_START_ADDRESS 
                        + fw_bytes_written, packet.data, packet.length);
                    fw_bytes_written += packet.length;

                    simple_timer_reset(&timer);
                    
                    if (fw_bytes_written >= fw_length) {
                        bl_state = BL_STATE_DONE;
                    } else {
                        comms_create_single_byte_packet(&packet, BL_PACKET_READY_FOR_DATA_DATA0);
                        comms_send_packet(&packet);
                    }
                } else {
                    check_update_timeout();
                }
            } break;

            case BL_STATE_DONE: {   
                shift_register_set_pattern(&sr1, 0xFF); 
                comms_create_single_byte_packet(&packet, 
                    BL_PACKET_UPDATE_SUCCESS_DATA0);
                comms_send_packet(&packet);

                // led_debug(DEBUG_4); // indicate update success
                
                system_delay(200); // arbitrary delay to allow packet to send

                gpio_teardown();
                uart_teardown();
                system_teardown();
                shift_register_teardown();

                if (validate_firmware_image()) {
                    jump_to_main();
                } else {
                    scb_reset_system(); // reset system if firmware is invalid
                }
            } break;

            default: {
                bl_state = BL_STATE_SYNC;
            } break;     
        }
    }

    jump_to_main();

    return 0;
}