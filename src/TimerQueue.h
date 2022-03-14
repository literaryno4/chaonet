//
// Created by chao on 2022/3/14.
//

#ifndef CHAONET_TIMERQUEUE_H
#define CHAONET_TIMERQUEUE_H

#include <set>
#include <vector>

#include "Channel.h"
#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"
#include "Timestamp.h"

using namespace muduo;

namespace chaonet {

class TimerQueue {
   public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);

   private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;

    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList  timers_;

};

}  // namespace chaonet

#endif  // CHAONET_TIMERQUEUE_H
