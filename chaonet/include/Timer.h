//
// Created by chao on 2022/3/14.
//

#ifndef CHAONET_TIMER_H
#define CHAONET_TIMER_H

#include <atomic>
#include <functional>

#include "utils/Timestamp.h"

namespace chaonet {

class Timer {
   public:
    typedef std::function<void()> TimerCallback;

    Timer(const TimerCallback& cb, Timestamp when, double interval)
        : callback_(cb),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(s_numCreated_.fetch_add(1)) {}

    Timer(const Timer&) = delete;

    void run() const { callback_(); }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(Timestamp now);

   private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static std::atomic_int64_t s_numCreated_;
};

}  // namespace chaonet

#endif  // CHAONET_TIMER_H
