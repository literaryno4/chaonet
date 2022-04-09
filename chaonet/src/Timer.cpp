//
// Created by chao on 2022/3/14.
//

#include "Timer.h"

using namespace chaonet;

std::atomic_int64_t Timer::s_numCreated_;

void Timer::restart(Timestamp now) {
    if (repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = Timestamp::invalid();
    }
}