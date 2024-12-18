#pragma once

#include <libopencm3/cm3/nvic.h>

volatile uint64_t ticks = 0;
void sys_tick_handler(void) {
    ++ticks;
}

static uint64_t get_ticks(void) {
    return ticks;
}