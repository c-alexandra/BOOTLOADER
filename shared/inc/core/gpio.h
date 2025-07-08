#pragma once

// Debug LED configuration
#define LED_PORT         (GPIOA)
#define LED_PIN          (GPIO1)

#define LED_PORT_BUILTIN (GPIOA)
#define LED_PIN_BUILTIN  (GPIO5)

// PWM configuration
#define PWM_PORT         (GPIOB)
#define PWM_PIN          (GPIO2)

// UART configuration
#define UART_PORT (GPIOA)
#define TX_PIN    (GPIO9)
#define RX_PIN    (GPIO10)

// Shift register debug LED configuration
#define SR_DEBUG_1 (0x1 << 0) // debug LED 1
#define SR_DEBUG_2 (0x1 << 1) // debug LED 2
#define SR_DEBUG_3 (0x1 << 2) // debug LED 3
#define SR_DEBUG_4 (0x1 << 3) // debug LED 4
#define SR_DEBUG_5 (0x1 << 4) // debug LED 5
#define SR_DEBUG_6 (0x1 << 5) // debug LED 6
#define SR_DEBUG_7 (0x1 << 6) // debug LED 7
#define SR_DEBUG_8 (0x1 << 7) // debug LED 8

#define SR_PORT   (GPIOA)
#define SR_DATA_PIN  (GPIO4)
#define SR_CLOCK_PIN (GPIO5)
#define SR_LATCH_PIN (GPIO6)


