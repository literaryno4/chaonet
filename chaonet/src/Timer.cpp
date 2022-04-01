//
// Created by chao on 2022/3/14.
//

#include "Timer.h"

using namespace chaonet;

void Timer::restart(Timestamp now) {
    if (repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = Timestamp::invalid();
    }
}