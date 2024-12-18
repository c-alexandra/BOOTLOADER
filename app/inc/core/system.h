#pragma once

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>

#define LED_PORT         (GPIOA)
#define LED_PORT_BUILTIN (GPIOA)

#define LED_PIN          (GPIO1)
#define LED_PIN_BUILTIN  (GPIO5)

#define CPU_FREQ         (84000000)
#define SYSTICK_FREQ     (1000)

static void rcc_setup(void) {
    // Set 3.3v clock to 84MHz
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void gpio_setup(void) {
    // enable rcc for GPIOA
    rcc_periph_clock_enable(RCC_GPIOA);
    // configure gpio port a, pin 5 for output, with no pull-up or down
    gpio_mode_setup(LED_PORT_BUILTIN | LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN_BUILTIN | LED_PIN);
}

static void systick_setup(void) {
    systick_set_frequency(SYSTICK_FREQ, CPU_FREQ); // 1000 / sec
    systick_counter_enable();
    systick_interrupt_enable();
}