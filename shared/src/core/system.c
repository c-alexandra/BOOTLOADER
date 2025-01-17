/*******************************************************************************
 * @file   system.c
 * @author Camille Alexandra
 *
 * @brief  Contains implementation for implementing various system-level 
 *         peripherals, such as RCC, GPIO, SYSTICK
 ******************************************************************************/

// External library includes
#include <libopencm3/stm32/rcc.h> // rcc_clock, rcc_periph
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h> // sys_tick_handler

// User includes
#include "common.h"
#include "core/system.h"

// Defines & Macros

// Global and Extern Declarations
static volatile uint64_t ticks = 0;

// Functions
void sys_tick_handler(void) {
    ++ticks;
}

uint64_t system_get_ticks(void) {
    return ticks;
}

static void rcc_setup(void) {
    // Set 3.3v clock to 84MHz
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
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
}

static void systick_setup(void) {
    systick_set_frequency(SYSTICK_FREQ, CPU_FREQ); // 1000 / sec
    systick_counter_enable();
    systick_interrupt_enable();
}

void system_setup(void) {
    rcc_setup();
    gpio_setup();
    systick_setup();
}