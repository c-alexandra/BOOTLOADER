/*******************************************************************************
 * @file   simple-timer.c   
 * @author camille aitken
 *
 * @brief
 ******************************************************************************/

#include "core/simple-timer.h"
#include "core/system.h"

/*******************************************************************************
 * @brief Initialize the values of a simple_timer_t object
 * @param timer Description of the first parameter.
 * @param wait_time Description of the second parameter.
 * @param auto_reset Description of the third parameter.
 ******************************************************************************/
void simple_timer_setup(simple_timer_t* timer, uint64_t wait_time, bool auto_reset) {
    timer->wait_time = wait_time;
    timer->auto_reset = auto_reset;
    timer->expired = false;
    
    timer->target_time = system_get_ticks() + wait_time;
}

/*******************************************************************************
 * @brief Check if the timer has expired
 * 
 * @param timer Pointer to the simple_timer_t object to check
 * @return True if the timer has expired, False otherwise
 ******************************************************************************/
bool simple_timer_check_has_expired(simple_timer_t* timer) {
    uint64_t now = system_get_ticks();
    bool has_expired = now >= timer->target_time; // check if past target time

    if (timer->expired) {
        return false;
    }

    if (has_expired) {
        if (timer->auto_reset) {
            uint64_t drift = now - timer->target_time; // how much we overshot
            timer->target_time = (now + timer->wait_time) - drift; // set the target time to the next wait time, accounting for overshoot
        } else {
            timer->expired = true;
        }
    }

    return has_expired;;
}

/*******************************************************************************
 * @brief Reset the timer to its initial state
 ******************************************************************************/
void simple_timer_reset(simple_timer_t* timer) {
    simple_timer_setup(timer, timer->wait_time, timer->auto_reset);
}

