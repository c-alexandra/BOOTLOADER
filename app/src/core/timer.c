#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>

#include "core/timer.h"

#define PRESCALER (84)
#define ARR_VALUE (1000)

// we want to setup PWM timer on PA2, PA2 can use timer 2, channel 3
void timer_setup(void) {
    rcc_periph_clock_enable(RCC_TIM2);

    // highlevel timer config
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    // setup pwm mode for given pin configuration
    timer_set_oc_mode(TIM2, TIM_OC4, TIM_OCM_PWM1);
    timer_enable_counter(TIM2);
    timer_enable_oc_output(TIM2, TIM_OC4);

    // setup frequency and resolution
    // set frequency to run at, set steps for given freq.
    // given clock freq. of 84_000_000
    // freq = system_freq / ((prescaler - 1) + (auto_reload - 1))
    timer_set_prescaler(TIM2, PRESCALER - 1);
    timer_set_period(TIM2, ARR_VALUE - 1);
}

// take in a duty cycle as a percentage
void timer_pwm_set_duty_cycle(float duty_cycle) {
    // duty cycle = (ccr / arr) * 100
    // duty cycle / 100 = ccr / arr
    // ccr arr * (duty cycle / 100)
    const float raw_value = (float)ARR_VALUE * (duty_cycle / 100.0f);

    timer_set_oc_value(TIM2, TIM_OC4, (uint32_t)raw_value);
}