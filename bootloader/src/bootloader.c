/*******************************************************************************
 * @file   bootloader.c
 * @author Camille Alexandra
 *
 * @brief  Redirect vector table to launch with custom bootloader
 ******************************************************************************/

// External library includes
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/stm32/rcc.h>
// #include <libopencm3/cm3/vector.h> // if using libopencm3 vector table

// User includes
#include "common.h"
#include "core/uart.h"
#include "core/system.h"
#include "comms.h"
#include "bl-flash.h"
#include "core/simple-timer.h"

// Defines & Macros
#define BOOTLOADER_SIZE        (0x8000U) // 32 768
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE)
#define MAX_FW_LENGTH          ((1024U * 512U) - BOOTLOADER_SIZE) // 512KB - bootloader size

#define DEVICE_ID (0xA3) // arbitrary device id used to identify for fw updates

#define SYNC_SEQUENCE_0 (0xC4)
#define SYNC_SEQUENCE_1 (0x55)
#define SYNC_SEQUENCE_2 (0x7E)
#define SYNC_SEQUENCE_3 (0x10)

#define DEFAULT_TIMEOUT (5000)  // default timeout at 5s
#define SHORT_TIMEOUT   (1000)  // short timeout at 1s
#define LONG_TIMEOUT    (15000) // long timeout at 15s

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
static uint32_t fw_bytes_written = 0; // track how many bytes have been written to flash
static uint8_t sync_seq[4] = {0};
static simple_timer_t timer; // module-level timer we will use for timeouts
static comms_packet_t packet; 

// Global and Extern Declarations

// Functions
static void jump_to_main(void) {
    // create typedef for a void function
    typedef void (*void_fn)(void);

    // reset vector is second entry in table, stack pointer is first, thus + 4
    uint32_t* reset_vector_entry = (uint32_t*)(MAIN_APP_START_ADDRESS + 4U);
    uint32_t* reset_vector = (uint32_t*)(*reset_vector_entry);

    // interpret reset_vector as void function and call function
    void_fn reset_fnc = (void_fn)reset_vector;

    reset_fnc();

    /**
     * The following implementation uses the libopencm3 vector table
     * and uses the libopencm3/cm3/vector.h file to call the reset function
     */ 
    // vector_table_t* vector_table = (vector_table_t*)MAIN_APP_START_ADDRESS;
    // main_vector_table->reset();
}

// breaks linker if rom set to 32kb
const uint8_t data[0x8000] = {0};
static void test_rom_size(void) {
    volatile uint8_t x = 0;
    for (uint32_t i = 0; i < 0x8000; ++i) {
        x += data[i];
    }
}

static void gpio_setup(void) {
    // enable rcc for GPIOA
    rcc_periph_clock_enable(RCC_GPIOA);

    //configure uart
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN | RX_PIN);
    gpio_set_af(UART_PORT, GPIO_AF7, TX_PIN | RX_PIN);

    gpio_mode_setup(UART_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
}

static void gpio_teardown(void) {
    gpio_mode_setup(UART_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, TX_PIN | RX_PIN);
    rcc_periph_clock_disable(RCC_GPIOA);
}

/**
 * @brief
 */
static void abort_fw_update(void) {
    comms_create_single_byte_packet(&packet, BL_PACKET_NACK_DATA0);
    comms_send_packet(&packet);
    bl_state = BL_STATE_DONE;
}

/** 
 * @brief Check if the firmware update sequence has timed out, and if so, abort
 *       the firmware update process
 */
static void check_update_timeout(void) {
    if (simple_timer_check_has_expired(&timer)) {
        abort_fw_update();
    }
}

/**
 * @brief Check if a given packet matches signature of device id packet
 * @note A device id packet will have a length of 2 bytes, with the first
 *       byte being BL_PACKET_DEVICE_ID_RESPONSE_DATA0, and the following
 *       byte being the device id
 * @param packet Pointer to the packet to check
 * @return True if the packet is a device id packet, False otherwise
 */
static bool is_device_id_packet(const comms_packet_t* packet) {
    if (packet->length != 2) {
        return false;
    }

    if (packet->data[0] != BL_PACKET_DEVICE_ID_RESPONSE_DATA0) {
        return false;
    }

    for (uint8_t i = 2; i < PACKET_DATA_LENGTH; ++i) {
        if (packet->data[i] != 0xFF) {
            return false;
        }
    }
    
    return true;
}

/** 
 * @brief Check if a given packet matches signature of firmware length packet
 * @note A firmware length packet will have a length of 5 bytes, with the first
 *       byte being BL_PACKET_FW_LENGTH_RESPONSE_DATA0, and the following 4
 *       corresponsing to a uint32_t firmware length
 * @param packet Pointer to the packet to check
 * @return True if the packet is a firmware length packet, False otherwise
 */
static bool is_fw_length_packet(const comms_packet_t* packet) {
    if (packet->length != 5) {
        return false;
    }

    if (packet->data[0] != BL_PACKET_FW_LENGTH_RESPONSE_DATA0) {
        return false;
    }

    for (uint8_t i = 5; i < PACKET_DATA_LENGTH; ++i) {
        if (packet->data[i] != 0xFF) {
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

    // initialize module level timer to check fw update timeouts
    simple_timer_setup(&timer, DEFAULT_TIMEOUT, false);

    while (true) {
        // handle separately from other states
        if (bl_state == BL_STATE_SYNC) {
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
                    comms_create_single_byte_packet(&packet, BL_PACKET_SYNC_OBSERVED_DATA0);
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
        comms_update();

        switch (bl_state) {
            case BL_STATE_UPDATE_REQ: {
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
                comms_create_single_byte_packet(&packet, BL_PACKET_DEVICE_ID_REQUEST_DATA0);
                comms_send_packet(&packet);
                bl_state = BL_STATE_DEVICE_ID_RESP;
            } break;

            case BL_STATE_DEVICE_ID_RESP: {
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
                comms_create_single_byte_packet(&packet, BL_PACKET_FW_LENGTH_REQUEST_DATA0);
                comms_send_packet(&packet);
                simple_timer_reset(&timer);
                bl_state = BL_STATE_FW_LENGTH_RESP;
            } break;

            case BL_STATE_FW_LENGTH_RESP: {
                if (comms_data_available()) {
                    comms_receive_packet(&packet);

                    fw_length = *((uint32_t*)&packet.data[1]);
                    // fw_length = (
                    //     (packet.data[1])      |
                    //     (packet.data[2]) << 8 |
                    //     (packet.data[3]) << 16|
                    //     (packet.data[4]) << 24
                    // );
                    
                    if (is_fw_length_packet(&packet) && fw_length <= MAX_FW_LENGTH) {
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
                bl_flash_erase_main_app(); // can take ~10s
                simple_timer_reset(&timer);
                bl_state = BL_STATE_RECEIVE_FW;
            } break;

            case BL_STATE_RECEIVE_FW: {
                if (comms_data_available()) {
                    comms_receive_packet(&packet);
                    
                    // write packet data to flash memory
                    bl_flash_write_main_app(MAIN_APP_START_ADDRESS + fw_bytes_written, packet.data, packet.length);
                    fw_bytes_written += packet.length;

                    simple_timer_reset(&timer);
                    
                    if (fw_bytes_written >= fw_length) {
                        bl_state = BL_STATE_DONE;
                    }
                } else {
                    check_update_timeout();
                }
            } break;

            case BL_STATE_DONE: {    
                comms_create_single_byte_packet(&packet, BL_PACKET_UPDATE_SUCCESS_DATA0);
                comms_send_packet(&packet);
                
                system_delay(200); // arbitrary delay to allow packet to send

                gpio_teardown();
                uart_teardown();
                system_teardown();

                jump_to_main();
            } break;

            default: {
                bl_state = BL_STATE_SYNC;
            } break;     
        }
    }

    jump_to_main();

    return 0;
}