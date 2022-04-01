//
// Created by chao on 2022/3/30.
//

#ifndef CHAONET_EVENTLOOPTHREADPOOL_H
#define CHAONET_EVENTLOOPTHREADPOOL_H

#include <memory>
#include <vector>

#include "EventLoopThread.h"

namespace chaonet {

class EventLoop;

class EventLoopThreadPool {
   public:
    explicit EventLoopThreadPool(EventLoop* baseLoop);
    ~EventLoopThreadPool() = default;
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start();
    EventLoop* getNextLoop();

   private:
    EventLoop* baseLoop_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};

}  // namespace chaonet

#endif  // CHAONET_EVENTLOOPTHREADPOOL_H
