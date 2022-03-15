//
// Created by chao on 2022/3/15.
//

#ifndef CHAONET_EVENTLOOPTHREAD_H
#define CHAONET_EVENTLOOPTHREAD_H

#include "Thread.h"
#include <mutex>
#include <condition_variable>

namespace chaonet {

class EventLoop;

class EventLoopThread {
   public:
    EventLoopThread();
    EventLoopThread(const EventLoopThread&) = delete;
    ~EventLoopThread();
    EventLoop* startLoop();

   private:
    void threadFunc();
    EventLoop* loop_;
    bool exiting_;
    muduo::Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

}

#endif  // CHAONET_EVENTLOOPTHREAD_H
