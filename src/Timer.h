//
// Created by chao on 2022/3/14.
//

#ifndef CHAONET_TIMER_H
#define CHAONET_TIMER_H

#include <functional>

#include "Timestamp.h"

namespace chaonet {

typedef std::function<void()> TimerCallback;

class Timer {
   public:
    Timer(const TimerCallback& cb, muduo::Timestamp when, double interval)
        : callback_(cb),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0) {}

    Timer(const Timer&) = delete;

    void run() const {
        callback_();
    }

    muduo::Timestamp expiration() const {return expiration_;}
    bool repeat() const { return repeat_;}

    void restart(muduo::Timestamp now);

   private:
    const TimerCallback callback_;
    muduo::Timestamp expiration_;
    const double interval_;
    const bool repeat_;
};

}  // namespace chaonet

#endif  // CHAONET_TIMER_H
