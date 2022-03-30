//
// Created by chao on 2022/3/15.
//

#include "EventLoopThread.h"

#include <functional>

#include "EventLoop.h"

using namespace chaonet;

EventLoopThread::EventLoopThread()
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this)),
      mutex_(),
      cond_() {}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    loop_->quit();
    thread_.join();
}

EventLoop* EventLoopThread::startLoop() {
    assert(!thread_.started());
    thread_.start();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait(lock);
        }
    }

    return loop_;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
}