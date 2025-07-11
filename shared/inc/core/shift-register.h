#pragma once

// Shift register debug LED configuration
#define SR_DEBUG_1 (0x1 << 0) // debug LED 1
#define SR_DEBUG_2 (0x1 << 1) // debug LED 2
#define SR_DEBUG_3 (0x1 << 2) // debug LED 3
#define SR_DEBUG_4 (0x1 << 3) // debug LED 4
#define SR_DEBUG_5 (0x1 << 4) // debug LED 5
#define SR_DEBUG_6 (0x1 << 5) // debug LED 6
#define SR_DEBUG_7 (0x1 << 6) // debug LED 7
#define SR_DEBUG_8 (0x1 << 7) // debug LED 8

#define SR_PORT      (GPIOB)
#define SR_DATA_PIN  (GPIO5) // MOSI pin for SPI -> SER pin on shift register
#define SR_CLOCK_PIN (GPIO3) // SCK pin for SPI -> SRCLK pin on shift register
#define SR_LATCH_PIN (GPIO0) // RCLK pin on shift register


void shift_register_setup(void);
void shift_register_set_pattern(uint8_t pattern);
void shift_register_set_led(uint8_t led, bool state);

uint8_t shift_register_get_state(void);
