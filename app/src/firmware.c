/*******************************************************************************
 * @file   firmware.c
 * @author Camille Alexandra
 *
 * @brief  main program driving the firmware portion of this STM32F446RE project
 ******************************************************************************/

// External library includes
#include <libopencm3/cm3/scb.h> // contains vector table offset register
#include <libopencm3/stm32/rcc.h> // contains relevant rcc timer functions

// User includes
#include "common.h"
#include "core/system.h"
#include "core/uart.h"
#include "timer.h"

// Defines & Macros
#define FLASH_MEM_BEGIN      (0x08000000)
#define FLASH_MEM_BOOTLOADER (0x08008000)
#define FLASH_MEM_END        (0x081FFFFF)

#define BOOTLOADER_SIZE (0x8000U) // 32KB

// DEBUG LED defines
#define DEBUG_1 (0x01) // debug LED 1
#define DEBUG_2 (0x02) // debug LED 2
#define DEBUG_3 (0x04) // debug LED 3
#define DEBUG_4 (0x08) // debug LED 4
#define DEBUG_5 (0x10) // debug LED 5
#define DEBUG_6 (0x20) // debug LED 6
#define DEBUG_7 (0x40) // debug LED 7
#define DEBUG_8 (0x80) // debug LED 8

#define DATA_PIN  (GPIO4)
#define CLOCK_PIN (GPIO5)
#define LATCH_PIN (GPIO6)

#define SR_PORT (GPIOA)

// Global and Extern Declarations

// Functions

/** 
 * @brief offset vector table location in memory by booloader size
 */
static void vector_setup(void) {
    SCB_VTOR = BOOTLOADER_SIZE;
}

static void gpio_setup(void) {
    // enable rcc for GPIOA
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    // configure gpio port a, pin 5 for output, with no pull-up or down
    gpio_mode_setup(LED_PORT_BUILTIN | LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN_BUILTIN | LED_PIN);

    // configure pwm on PB2
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
    gpio_set_af(GPIOB, GPIO_AF1, GPIO2);

    //configure uart
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN | RX_PIN);
    gpio_set_af(UART_PORT, GPIO_AF7, TX_PIN | RX_PIN);

    // configure pins for DEBUG LEDs on 8-bit shift register
    // set pins to output mode, no pull-up or down
    gpio_mode_setup(SR_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DATA_PIN | CLOCK_PIN | LATCH_PIN); 

    // gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO4); // push-pull output, 2MHz speed

    // initlialize all pins low
    gpio_clear(SR_PORT, DATA_PIN | CLOCK_PIN | LATCH_PIN); 
}

static void shift_LED(uint8_t pattern) {
    // latch low during shifting
    gpio_clear(SR_PORT, LATCH_PIN);

    // shift 8 bits MSB first
    for (uint8_t i = 7; i >= 0; --i) {
        // set data pin to the current bit
        if (pattern & (1 << i)) {
            gpio_set(SR_PORT, DATA_PIN);
        } else {
            gpio_clear(SR_PORT, DATA_PIN);
        }

        // toggle clock pin to shift in the bit
        // TODO: use a delay here to ensure the shift register has time to shift in the bit
        gpio_set(SR_PORT, CLOCK_PIN);
        gpio_clear(SR_PORT, CLOCK_PIN);
    }

    // latch data to output pins
    gpio_set(SR_PORT, LATCH_PIN);
    gpio_clear(SR_PORT, LATCH_PIN);
}

static void blink_led(void) {
    uint64_t start_time = system_get_ticks();

    if ((system_get_ticks() - start_time) >= 1000) {
        gpio_toggle(LED_PORT_BUILTIN | LED_PORT, LED_PIN_BUILTIN | LED_PIN);
        start_time = system_get_ticks();
    }
}

int main(void) {
    system_setup();
    gpio_setup();
    timer_setup();
    vector_setup();
    uart_setup();

    uint64_t start_time = system_get_ticks();
    uint64_t pwm_time = system_get_ticks();

    float cycle = 0.0;

    while (1) {
        blink_led();
        if ((system_get_ticks() - pwm_time) >= 100) {
            if (cycle == 100.0) {
                cycle = 0.0;
            }
            timer_pwm_set_duty_cycle(cycle);
            cycle += 10.0;
            pwm_time = system_get_ticks();
        }

        if (uart_data_available()) {
            uint8_t data = uart_receive_byte();
            uart_send_byte(data);
        }

        // system_delay(1000);
    }

    return 0;
}