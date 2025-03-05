#pragma once

#include "common.h"

typedef struct simple_timer_t {
    uint64_t wait_time;   // how long to wait before *something*
    uint64_t target_time; // when the timer will expire (now + wait_time)

    bool expired;         // has the timer expired
    bool auto_reset;      // should the timer reset itself after expiring
} simple_timer_t;

void simple_timer_setup(simple_timer_t* timer, uint64_t wait_time, bool auto_reset);
bool simple_timer_check_has_expired(simple_timer_t* timer);
void simple_timer_reset(simple_timer_t* timer);

