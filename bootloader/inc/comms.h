#pragma once

#include "../shared/inc/common.h"

#define PACKET_DATA_LENGTH (16)

typedef struct comms_packet_t {
    uint8_t length;
    uint8_t data[16];
    uint8_t crc;
} comms_packet_t;

void comms_setup(void);
void comms_send_packet(comms_packet_t* packet);
bool comms_receive_packet(comms_packet_t* packet);