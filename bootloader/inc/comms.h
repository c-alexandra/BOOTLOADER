#pragma once

#include "common.h"

#define PACKET_LENGTH_LENGTH  (1)
#define PACKET_DATA_LENGTH    (16)
#define PACKET_CRC_LENGTH     (1)
#define PACKET_LENGTH         (PACKET_LENGTH_LENGTH + PACKET_DATA_LENGTH + PACKET_CRC_LENGTH)

#define PACKET_RETX_DATA0 (0x19)
#define PACKET_ACK_DATA0  (0x15)

// Firmware update sequence special characters
// TODO: Create a table for special packet type designations
#define BL_PACKET_SYNC_OBSERVED_DATA0              (0x20)
#define BL_PACKET_FW_UPDATE_REQUEST_DATA0          (0x31)
#define BL_PACKET_FW_UPDATE_RESPONSE_DATA0         (0x37)
#define BL_PACKET_DEVICE_ID_REQUEST_DATA0          (0x3C)
#define BL_PACKET_DEVICE_ID_RESPONSE_DATA0         (0x3F)
#define BL_PACKET_FW_LENGTH_REQUEST_DATA0          (0x42)
#define BL_PACKET_FW_LENGTH_RESPONSE_DATA0         (0x45)
#define BL_PACKET_READY_FOR_DATA_DATA0             (0x48)
#define BL_PACKET_UPDATE_SUCCESS_DATA0             (0x54)
#define BL_PACKET_NACK_DATA0                       (0x99)

typedef struct comms_packet_t {
    uint8_t length;
    uint8_t data[PACKET_DATA_LENGTH];
    uint8_t crc;
} comms_packet_t;

void comms_setup(void);
void comms_update(void);

bool comms_is_single_byte_packet(comms_packet_t* packet, uint8_t data0);

bool comms_data_available(void);
void comms_send_packet(comms_packet_t* packet);
void comms_receive_packet(comms_packet_t* packet);
void comms_create_single_byte_packet(comms_packet_t* packet, uint8_t data0);

uint8_t comms_compute_crc(comms_packet_t* packet);