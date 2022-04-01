//
// Created by chao on 2022/3/30.
//

#include "EventLoopThreadPool.h"

#include "EventLoop.h"
#include "EventLoopThread.h"
#include <assert.h>

using namespace chaonet;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
    : baseLoop_(baseLoop), started_(false), numThreads_(0), next_(0) {}

void EventLoopThreadPool::start() {
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;
    for (int i = 0; i < numThreads_; ++i) {
        threads_.push_back(std::make_unique<EventLoopThread>());
        loops_.push_back(threads_[i]->startLoop());
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    baseLoop_->assertInLoopThread();
    EventLoop* loop = baseLoop_;
    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (static_cast<size_t>(next_) >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}
