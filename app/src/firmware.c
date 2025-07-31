/*******************************************************************************
 * @file   firmware.c
 * @author Camille Alexandra
 *
 * @brief  Main program driving the firmware portion of this STM32F446RE project
 ******************************************************************************/

#include <libopencm3/cm3/scb.h> // contains vector table offset register
#include <libopencm3/stm32/rcc.h> // contains relevant rcc timer functions
#include <libopencm3/stm32/spi.h>

#include "common.h"
#include "core/system.h"
#include "core/uart.h"
#include "core/gpio.h"
#include "timer.h"
#include "core/shift-register.h"
#include "core/firmware-info.h"

/*******************************************************************************
 * @brief offset vector table location in memory by booloader size
 ******************************************************************************/
static void vector_setup(void) {
    SCB_VTOR = BOOTLOADER_SIZE;
}

/*******************************************************************************
 * @brief Initializes the GPIO pins for the application
 ******************************************************************************/
static void gpio_setup(void) {
    // enable rcc for GPIOA
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    // configure gpio port a, pin 5 for output, with no pull-up or down
    gpio_mode_setup(LED_PORT_BUILTIN | LED_PORT, GPIO_MODE_OUTPUT, 
        GPIO_PUPD_NONE, LED_PIN_BUILTIN | LED_PIN);

    // configure pwm on PB2
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
    gpio_set_af(GPIOB, GPIO_AF1, GPIO2);

    //configure uart
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN | RX_PIN);
    gpio_set_af(UART_PORT, GPIO_AF7, TX_PIN | RX_PIN);

    gpio_set(LED_PORT_BUILTIN, LED_PIN_BUILTIN); // set builtin LED high
}

/*******************************************************************************
 * @brief Blinks led on a given systick interval
 * 
 * @param offset The time in milliseconds to wait before toggling the LED
 ******************************************************************************/
static void blink_led(uint16_t offset, uint64_t *start_time) {
    if ((system_get_ticks() - *start_time) >= offset) {
        gpio_toggle(LED_PORT, LED_PIN);
        *start_time = system_get_ticks();
    }
}

/*******************************************************************************
 * @brief Increments the duty cycle of the PWM LED in a breathing pattern
 * 
 * @param offset The time in milliseconds to acheive brightness extrema
 ******************************************************************************/
static void led_breathe(uint16_t offset, uint64_t *pwm_time) {
    static float cycle = 0.0;
    static bool increasing = true;

    if ((system_get_ticks() - *pwm_time) >= offset) {
        if (increasing) {
            cycle += 10.0;
        } else {
            cycle -= 10.0;
        }

        if (cycle >= 100.0) {
            increasing = false;
        } else if (cycle <= 0.0) {
            increasing = true;
        }
        
        timer_pwm_set_duty_cycle(cycle);
        *pwm_time = system_get_ticks();
    }
}

/*******************************************************************************
 * @brief retransmits the last received byte over UART
 ******************************************************************************/
static void uart_retransmit(void) {
    // retransmit the last received byte over UART
    if (uart_data_available()) {
        uint8_t data = uart_receive_byte();
        uart_send_byte(data);
    }
}

/*******************************************************************************
 * @brief Walks through the shift register LEDs, advancing the state every 
 *        offset milliseconds
 * 
 * @param sr Pointer to the structure containing SR configuration
 * @param offset The time in milliseconds to wait before advancing the SR state
 * @param start_time Pointer to the start time for the walking operation
 ******************************************************************************/
static void walk(ShiftRegister8_t *sr, uint16_t offset, uint64_t *start_time) {
    if ((system_get_ticks() - *start_time) >= offset) {
        shift_register_advance(sr); // advance the shift register state
        *start_time = system_get_ticks();
    }
}

int main(void) {
    system_setup();
    gpio_setup();
    timer_setup();
    vector_setup();
    uart_setup();

    ShiftRegister8_t sr1 = {
        .led_state = 0x00, // start with first LED on
        .num_outputs = 8, // 8 outputs for the debug LEDs
        .gpio_port = SR1_PORT,
        .ser_pin = SR1_DATA_PIN,
        .srclk_pin = SR1_CLOCK_PIN,
        .rclk_pin = SR1_LATCH_PIN
    };

    shift_register_setup(&sr1);

    uint64_t start_time = system_get_ticks();
    uint64_t pwm_time = system_get_ticks();
    uint64_t sr_time = system_get_ticks();

    // float cycle = 0.0;

    while (1) {
        blink_led(1000, &start_time); // blink led every second
        led_breathe(100, &pwm_time); // breathe led every second

        // update shift register with current state
        // shift_register_set_pattern(&sr1, 0x02); 
        
        walk(&sr1, 1000, &sr_time); // walk through the shift register LEDs
       
        uart_retransmit();
    }

    return 0;
}