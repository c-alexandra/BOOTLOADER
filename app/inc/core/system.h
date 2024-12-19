#pragma once

#include <libopencm3/stm32/gpio.h>

#include "common.h"

#define LED_PORT         (GPIOA)
#define LED_PORT_BUILTIN (GPIOA)

#define LED_PIN          (GPIO1)
#define LED_PIN_BUILTIN  (GPIO5)

#define CPU_FREQ         (84000000)
#define SYSTICK_FREQ     (1000)

void system_setup(void);
uint64_t system_get_ticks(void);