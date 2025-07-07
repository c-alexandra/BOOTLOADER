/*******************************************************************************
 * @file   comms.c
 * @author Camille Aitken
 *
 * @brief
 ******************************************************************************/

// External library includes

// User includes
#include "comms.h"
#include "core/uart.h"
#include "core/crc.h"

// Defines & macros
#define PACKET_BUFFER_LENGTH (8)

//------------------------------------------------------------------------------
// Global and Extern Declarations

// 
typedef enum comms_state_t {
    CommsState_Length,
    CommsState_Data,
    CommsState_CRC
} comms_state_t;

typedef struct comms_ring_buffer_t {
    comms_packet_t* buffer;
    uint32_t mask;
    uint32_t head;
    uint32_t tail;
} comms_ring_buffer_t;

static comms_state_t state = CommsState_Length;
static uint8_t data_index = 0;

// temp packet for storing data
static comms_packet_t temp_packet = { .length = 0, .data = {0}, .crc = 0 };
static comms_packet_t retx_packet = { .length = 0, .data = {0}, .crc = 0 };
static comms_packet_t ack_packet = { .length = 0, .data = {0}, .crc = 0 };
static comms_packet_t last_transmit_packet = { 
    .length = 0, .data = {0}, .crc = 0 
}; 

static comms_packet_t packet_buffer[PACKET_BUFFER_LENGTH] = {0U};
static comms_ring_buffer_t packet_ring_buffer = { .buffer = packet_buffer, .mask = PACKET_BUFFER_LENGTH - 1, .head = 0, .tail = 0 };

//------------------------------------------------------------------------------
// Functions

/*******************************************************************************
 * @brief Copy the contents of one packet to another
 * 
 * @param src Pointer to the source packet
 * @param dest Pointer to the destination packet
 ******************************************************************************/
static void comms_packet_memcpy(const comms_packet_t* src, comms_packet_t* dest) {
    dest->length = src->length;
    for (uint8_t i = 0; i < PACKET_DATA_LENGTH; ++i) {
        dest->data[i] = src->data[i];
    }
    dest->crc = src->crc;
}

/*******************************************************************************
 * @brief Check if a given packet matches a specially defined packet
 * 
 * @param packet Pointer to the packet to check
 * @param special_packet Pointer to the special packet to compare against
 * @return True if the packet matches a special packet, False otherwise
 ******************************************************************************/
static bool comms_is_special_packet(comms_packet_t* packet, 
    comms_packet_t* special_packet) {
    for (uint8_t i = 0; i < (PACKET_LENGTH - PACKET_CRC_LENGTH); ++i) {
        if (((uint8_t*)packet)[i] != ((uint8_t*)special_packet)[i]) {
            return false;
        }
    }

    return true;
}

/*******************************************************************************
 * @brief Check if a given packet is a single byte packet
 * 
 * @param packet Pointer to the packet to check
 * @param data0 The data byte to check against
 * @return True if the packet is a single byte packet, False otherwise
 ******************************************************************************/
bool comms_is_single_byte_packet(comms_packet_t* packet, uint8_t data0) {
    if (packet->length != 1) {
        return false;
    }

    if (packet->data[0] != data0) {
        return false;
    }

    for (uint8_t i = 1; i < PACKET_DATA_LENGTH; ++i) {
        if (packet->data[i] != 0xFF) {
            return false;
        }
    }
    
    return true;
}

/*******************************************************************************
 * @brief Setup the communication peripheral
 ******************************************************************************/
void comms_setup(void) {
    comms_create_single_byte_packet(&retx_packet, PACKET_RETX_DATA0);
    comms_create_single_byte_packet(&ack_packet, PACKET_ACK_DATA0);
}

/*******************************************************************************
 * @brief Receive UART data and parse it into packets
 * 
 * @note  This function implements a communication state machine which parses 
 *        incoming UART data into readable packets, then stores 
 *        them in a ring buffer structure
 ******************************************************************************/
void comms_update(void) {
    while (uart_data_available()) {
        switch (state) {
            case CommsState_Length: {
                temp_packet.length = uart_receive_byte();
                state = CommsState_Data;
            } break;

            case CommsState_Data: {
                if (data_index < PACKET_DATA_LENGTH) {
                    temp_packet.data[data_index] = uart_receive_byte();
                    data_index++;
                } else {
                    data_index = 0;
                    state = CommsState_CRC;
                }
            } break;

            case CommsState_CRC: {
                temp_packet.crc = uart_receive_byte();
                uint8_t calculated_crc = comms_compute_crc(&temp_packet);

                // check if received packet was corrupted
                if (temp_packet.crc != calculated_crc) {
                    comms_send_packet(&retx_packet);
                    state = CommsState_Length;
                    break;
                } 

                // check if received packet was retx packet
                if (comms_is_special_packet(&temp_packet, &retx_packet)) {
                    comms_send_packet(&last_transmit_packet);
                    state = CommsState_Length;
                    break;
                }

                // check if received packet was ack packet
                if (comms_is_special_packet(&temp_packet, &ack_packet)) {
                    state = CommsState_Length;
                    break;
                }

                // packet was good, store it in the ring buffer

                // assert that the ring buffer is not full
                uint32_t next_write_index = (packet_ring_buffer.tail + 1) & packet_ring_buffer.mask;
                // replace the std assert() call because it broke stuff
                if (next_write_index == packet_ring_buffer.head) {
                    __asm__("BKPT #0");
                }

                comms_packet_memcpy(&temp_packet, &packet_ring_buffer.buffer[packet_ring_buffer.tail]);
                packet_ring_buffer.tail = next_write_index;
                comms_send_packet(&ack_packet);
                state = CommsState_Length;
            } break;

            default: {
                state = CommsState_Length;
            } break;
        }
    }
}

/*******************************************************************************
 * @brief Check if data is available in the communication peripheral
 * 
 * @return True if data is available, False otherwise
 ******************************************************************************/
bool comms_data_available(void) {
    return (packet_ring_buffer.head != packet_ring_buffer.tail);
}

/*******************************************************************************
 * @brief Send a packet of data
 * 
 * @param packet Pointer to the packet to send
 ******************************************************************************/
void comms_send_packet(comms_packet_t* packet) {
    uart_send((uint8_t*)packet, PACKET_LENGTH);
    comms_packet_memcpy(packet, &last_transmit_packet);
}

/*******************************************************************************
 * @brief Receive a packet of data
 * 
 * @param packet Pointer to packet buffer to write into
 ******************************************************************************/
void comms_receive_packet(comms_packet_t* packet) {
    comms_packet_memcpy(&packet_ring_buffer.buffer[packet_ring_buffer.head], 
        packet);
    packet_ring_buffer.head = (packet_ring_buffer.head + 1) 
    & packet_ring_buffer.mask;
}

/*******************************************************************************
 * @brief Create a single byte packet
 * 
 * @param packet Pointer to the packet to create
 * @param data0 The data byte to write into the packet
 ******************************************************************************/
void comms_create_single_byte_packet(comms_packet_t* packet, uint8_t data0) {
    // Alternatively, use memset() to set all bytes to 0xFF
    // memset(packet, 0xFF, sizeof(comms_packet_t));

    packet->length = 1;
    packet->data[0] = data0;
    for (uint8_t i = 1; i < PACKET_DATA_LENGTH; ++i) {
        packet->data[i] = 0xFF;
    }
    packet->crc = comms_compute_crc(packet);
}

/*******************************************************************************
 * @brief Compute the CRC for a given packet
 * 
 * @param packet Pointer to the packet to compute the CRC for
 * @return The computed CRC value
 ******************************************************************************/
uint8_t comms_compute_crc(comms_packet_t* packet) {
    uint8_t crc = crc8((uint8_t*)packet, PACKET_LENGTH - PACKET_CRC_LENGTH);

    return crc;
}
