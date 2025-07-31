#pragma once

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/memorymap.h> // FLASH_BASE

#include "common.h"

#define CPU_FREQ         (84000000) // 84MHz
#define SYSTICK_FREQ     (1000)

void system_setup(void);
uint64_t system_get_ticks(void);

void system_delay(uint64_t milliseconds);
void system_teardown(void);