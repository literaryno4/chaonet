//
// Created by chao on 2022/3/13.
//

#ifndef CHAONET_EVENTLOOP_H
#define CHAONET_EVENTLOOP_H

#include <vector>
#include <mutex>

#include "CurrentThread.h"
#include "Thread.h"
#include "Timestamp.h"
#include "TimerId.h"
#include "Callbacks.h"

using namespace muduo;

namespace chaonet {

class Channel;
class Poller;
class TimerQueue;

class EventLoop {
   public:
    typedef std::function<void()> Functor;
    EventLoop();
    EventLoop(const EventLoop&) = delete;
    ~EventLoop();

    void loop();

    void quit();

    Timestamp pollReturnTime() const { return pollReturnTimer_;}

    void runInLoop(const Functor& cb);

    void queueInLoop(const Functor& cb);

    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    TimerId runAfter(double delay, const TimerCallback& cb);
    TimerId runEvery(double interval, const TimerCallback& cb);

    void wakeup() const;

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
    void handleRead() const;
    void doPendingFunctors();

    typedef std::vector<Channel*> ChannelList;

    bool looping_;
    bool quit_;
    bool callingPendingFunctors_;
    const pid_t threadId_;
    Timestamp pollReturnTimer_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;
    std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;
};

}  // namespace chaonet

#endif  // CHAONET_EVENTLOOP_H
