//
// Created by chao on 2022/3/13.
//

#ifndef CHAONET_EVENTLOOP_H
#define CHAONET_EVENTLOOP_H

#include <vector>

#include "CurrentThread.h"
#include "Thread.h"
#include "Timestamp.h"
#include "TimerId.h"
#include "Timer.h"

using namespace muduo;

namespace chaonet {

class Channel;
class Poller;
class TimerQueue;

class EventLoop {
   public:
    EventLoop();
    EventLoop(const EventLoop&) = delete;
    ~EventLoop();

    void loop();

    void quit();

    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    TimerId runAfter(double delay, const TimerCallback& cb);
    TimerId runEvery(double interval, const TimerCallback& cb);

    void updateChannel(Channel* channel);

    void assertInLoopThread() {
        if (!isInLoopThread()) {
            abortNotInLoopThread();
        }
    }

    bool isInLoopThread() const {
        return threadId_ == muduo::CurrentThread::tid();
    }

   private:
    void abortNotInLoopThread();

    typedef std::vector<Channel*> ChannelList;

    bool looping_;
    bool quit_;
    const pid_t threadId_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    ChannelList activeChannels_;
};

}  // namespace chaonet

#endif  // CHAONET_EVENTLOOP_H
