#pragma once

#include "../../shared/inc/common.h"

typedef struct ring_buffer_t {
    uint8_t* buffer;
    uint32_t mask;
    uint32_t head;
    uint32_t tail;
} ring_buffer_t;

void ring_buffer_setup(ring_buffer_t* rb, uint8_t* buffer, uint32_t size);
bool ring_buffer_empty(ring_buffer_t* rb);
bool ring_buffer_write(ring_buffer_t* rb, uint8_t data);
bool ring_buffer_read(ring_buffer_t* rb, uint8_t* data);