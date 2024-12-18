// library includes


// project headers
#include "../inc/common.h"
#include "../inc/core/system.h"
#include "../inc/core/handlers.h"


// toggle output of GPIO from high to low, regardless of previous state
static void toggle_led(void) {
    gpio_toggle(LED_PORT_BUILTIN | LED_PORT, LED_PIN_BUILTIN | LED_PIN);
}

int main(void) {
    rcc_setup();
    gpio_setup();
    systick_setup();

    uint64_t start_time = get_ticks();

    while (1) {
        if ((get_ticks() - start_time) >= 1000) {
            toggle_led();
            start_time = get_ticks();
        }
    }

    return 0;
}