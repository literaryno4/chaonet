//
// Created by chao on 2022/3/14.
//

#include "Timer.h"

using namespace chaonet;

void Timer::restart(muduo::Timestamp now) {
    if (repeat_) {
        expiration_ = muduo::addTime(now, interval_);
    } else {
        expiration_ = muduo::Timestamp::invalid();
    }
}