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

static void systick_setup(void) {
    systick_set_frequency(SYSTICK_FREQ, CPU_FREQ); // 1000 / sec
    systick_counter_enable();
    systick_interrupt_enable();
}

void system_setup(void) {
    rcc_setup();
    systick_setup();
}

/** 
 * @brief Delays the whole system for a given number of milliseconds
 * 
 * @param milliseconds The number of milliseconds to delay the system
 */
void system_delay(uint64_t milliseconds) {
    uint64_t start_time = system_get_ticks();
    while ((system_get_ticks() - start_time) < milliseconds) {
        // do nothing
        // can't be optimized out because the ticks are volatile
    }
}

void system_teardown(void) {
    systick_interrupt_disable();
    systick_counter_disable();
    systick_clear();
    // rcc_periph_clock_disable(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}