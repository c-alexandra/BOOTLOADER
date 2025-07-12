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

#define SR1_PORT      (GPIOB)
#define SR1_DATA_PIN  (GPIO5) // MOSI pin for SPI -> SER pin on shift register
#define SR1_CLOCK_PIN (GPIO3) // SCK pin for SPI -> SRCLK pin on shift register
#define SR1_LATCH_PIN (GPIO0) // RCLK pin on shift register

typedef struct {
    uint8_t  led_state; // current state of the SR LEDs
    uint16_t num_outputs; // number of utilized outputs in the SR
    uint32_t gpio_port; // GPIO port used for the SR
    uint16_t rclk_pin; // GPIO pin used for latching data into the SR
    uint16_t srclk_pin; // GPIO pin used for clocking data into the SR
    uint16_t ser_pin; // GPIO pin used for serial data input to the SR
} ShiftRegister8_t;

void shift_register_setup(const ShiftRegister8_t *sr);
void shift_register_set_pattern(ShiftRegister8_t *sr, uint8_t pattern);
void shift_register_set_led(ShiftRegister8_t *sr, uint8_t led, bool state);
void shift_register_advance(ShiftRegister8_t *sr);

void shift_register_teardown(void);