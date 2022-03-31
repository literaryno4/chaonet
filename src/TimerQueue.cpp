//
// Created by chao on 2022/3/14.
//

#include "TimerQueue.h"

#include <unistd.h>
#include <sys/timerfd.h>

#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"
#include <spdlog/spdlog.h>

namespace chaonet {
namespace detail {

int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        SPDLOG_ERROR("Failed in timerfd_create");
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch() -
                           Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100) {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec =
        static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    SPDLOG_TRACE("TimerQueue::handleRead() {} at {}", howmany,
              now.toString());
    if (n != sizeof(howmany)) {
        SPDLOG_ERROR ("TimerQueue::handleRead() reads {} bytes instead of 8", n);
    }
}

void resetTimerfd(int timerfd, Timestamp expiration) {
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof(newValue));
    bzero(&oldValue, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret) {
        SPDLOG_ERROR("timerfd_settime()");
    }
}
}  // namespace detail

using namespace chaonet;
using namespace chaonet::detail;

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_() {
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    ::close(timerfd_);
}

TimerId TimerQueue::addTimer(const TimerCallback &cb, Timestamp when, double interval) {
    TimerPtr timer = std::make_shared<Timer>(cb, when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));

    return TimerId(timer.get());
}

void TimerQueue::addTimerInLoop(TimerPtr timer) {
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if (earliestChanged) {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);
    std::vector<Entry> expired = getExpired(now);

    for(auto it = expired.begin(); it != expired.end(); ++it) {
        it->second->run();
    }

    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    std::vector<Entry> expired;
    Entry sentry = std::make_pair(now, nullptr);
    auto it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);
    std::copy(timers_.begin(), it, std::back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now) {
    Timestamp nextExpire;
    for (auto it = expired.begin(); it != expired.end(); ++it) {
        if (it->second->repeat()) {
            it->second->restart(now);
            insert(it->second);
        }
    }

    if (!timers_.empty()) {
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid()) {
        resetTimerfd(timerfd_, nextExpire);
    }
}

bool TimerQueue::insert(TimerPtr timer) {
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    auto it = timers_.begin();
    if (it == timers_.end() || when < it->first) {
        earliestChanged = true;
    }
    auto result = timers_.insert(std::make_pair(when, timer));
    assert(result.second);
    return earliestChanged;
}

}  // namespace chaonet