// library includes


// project headers
#include "common.h"
#include "core/system.h"
#include "core/timer.h"

// toggle output of GPIO from high to low, regardless of previous state
static void toggle_led(void) {
    gpio_toggle(LED_PORT_BUILTIN | LED_PORT, LED_PIN_BUILTIN | LED_PIN);
}

int main(void) {
    system_setup();
    timer_setup();

    uint64_t start_time = system_get_ticks();
    uint64_t pwm_time = system_get_ticks();

    float cycle = 0.0;

    while (1) {
        if ((system_get_ticks() - start_time) >= 1000) {
            toggle_led();
            start_time = system_get_ticks();
        }
        if ((system_get_ticks() - pwm_time) >= 100) {
            if (cycle == 100.0) {
                cycle = 0.0;
            }
            timer_pwm_set_duty_cycle(cycle);
            cycle += 5.0;
            pwm_time = system_get_ticks();
        }
    }

    return 0;
}