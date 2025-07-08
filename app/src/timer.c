/*******************************************************************************
 * @file   timer.c
 * @author Camille Alexandra
 *
 * @brief  Implements timers, used in this case for PWM
 ******************************************************************************/

// External library includes
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>

// User includes
#include "timer.h"

// Defines & macros
#define PRESCALER (84)
#define ARR_VALUE (1000)

/*******************************************************************************
 * @brief Setup timer for PWM output
 * 
 * @note we want to setup PWM timer on PB2.
 *       By checking datasheet, we see PB2 can use timer 2, channel 4
 ******************************************************************************/
void timer_setup(void) {
    rcc_periph_clock_enable(RCC_TIM2); // enable rcc for timer 2

    // write to ctrl reg. timer 2, no clock division, edge aligned, up counting
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    // setup pwm mode for given pin configuration
    timer_set_oc_mode(TIM2, TIM_OC4, TIM_OCM_PWM1); // output compare channel 4
    timer_enable_counter(TIM2);
    timer_enable_oc_output(TIM2, TIM_OC4);

    // setup frequency and resolution
    // set frequency to run at, set steps for given freq.
    // given clock freq. of 84_000_000
    // freq = system_freq / ((prescaler - 1) + (auto_reload - 1))
    timer_set_prescaler(TIM2, PRESCALER - 1);
    timer_set_period(TIM2, ARR_VALUE - 1);
}

/*******************************************************************************
 * @brief Set the duty cycle for the PWM output
 * 
 * @param duty_cycle The desired duty cycle as a percentage (0.0 to 100.0)
 ******************************************************************************/
void timer_pwm_set_duty_cycle(float duty_cycle) {
    // duty cycle = (ccr / arr) * 100
    // duty cycle / 100 = ccr / arr
    // ccr arr * (duty cycle / 100)
    const float raw_value = (float)ARR_VALUE * (duty_cycle / 100.0f);

    timer_set_oc_value(TIM2, TIM_OC4, (uint32_t)raw_value);
}