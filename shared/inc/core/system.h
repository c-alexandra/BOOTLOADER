#pragma once

#include <libopencm3/stm32/gpio.h>

#include "common.h"

#define LED_PORT         (GPIOA)
#define LED_PORT_BUILTIN (GPIOA)

#define LED_PIN          (GPIO1)
#define LED_PIN_BUILTIN  (GPIO5)

#define PWM_PORT         (GPIOB)
#define PWM_PIN          (GPIO2)

#define CPU_FREQ         (84000000)
#define SYSTICK_FREQ     (1000)

#define UART_PORT (GPIOA)
#define TX_PIN    (GPIO9)
#define RX_PIN    (GPIO10)

void system_setup(void);
uint64_t system_get_ticks(void);