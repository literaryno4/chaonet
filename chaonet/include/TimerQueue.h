//
// Created by chao on 2022/3/14.
//

#ifndef CHAONET_TIMERQUEUE_H
#define CHAONET_TIMERQUEUE_H

#include <set>
#include <vector>
#include <memory>

#include "Channel.h"
#include "utils/Timestamp.h"
#include "Callbacks.h"

namespace chaonet {

class EventLoop;
class Timer;
class TimerId;

class TimerQueue {
   public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);
    void cancel(TimerId timerId);

   private:
    typedef std::shared_ptr<Timer> TimerPtr;
    typedef std::pair<Timestamp, TimerPtr> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::pair<TimerPtr, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(TimerPtr timer);
    void cancelInLoop(TimerId timerId);
    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(TimerPtr timer);

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList  timers_;

    // for cancel()
    bool callingExpiredTimers_;
    ActiveTimerSet activeTimers_;
    ActiveTimerSet cancelingTimers_;
};

}  // namespace chaonet

#endif  // CHAONET_TIMERQUEUE_H
