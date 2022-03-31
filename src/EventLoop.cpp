//
// Created by chao on 2022/3/13.
//

#include "EventLoop.h"

#include <signal.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <unistd.h>

#include <cassert>

#include "Channel.h"
#include "Poller.h"
#include "TimerQueue.h"
#include <spdlog/spdlog.h>

using namespace chaonet;

class IgnoreSigPipe {
   public:
    IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
};

// IgnoreSigPipe initObj;

__thread EventLoop *t_loopInThisThread = nullptr;
const int kPollTimeMs = 10000;

static int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        SPDLOG_ERROR("Failed in eventfd");
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(::syscall(SYS_gettid)),
      poller_(new Poller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
    SPDLOG_TRACE("EventLoop created {} in thread {}", this, threadId_);
    if (t_loopInThisThread) {
        SPDLOG_ERROR("Another EventLoop exists in this thread {}", threadId_);
    } else {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    assert(!looping_);
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_) {
        activeChannels_.clear();
        pollReturnTimer_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (auto &activeChannel : activeChannels_) {
            activeChannel->handleEvent(pollReturnTimer_);
        }
        doPendingFunctors();
    }

    SPDLOG_TRACE("EventLoop stop looping");
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(const Functor &cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor &cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }

    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Timestamp &time, const TimerCallback &cb) {
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}
TimerId EventLoop::runEvery(double interval, const TimerCallback &cb) {
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::updateChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread() {
    SPDLOG_ERROR(
        "EventLoop::abortNotInLoopThread - EventLoop was created in threadId_ "
        "= {}, current thread id = ",
        threadId_, ::syscall(SYS_gettid));
}

void EventLoop::wakeup() const {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof(one)) {
        SPDLOG_ERROR(
            "EventLoop::handleRead() reads {} "
            "bytes instead of 8",
            n);
    }
}

void EventLoop::handleRead() const {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        SPDLOG_ERROR(
            "EventLoop::handleRead() read {}"
            " bytes instead of 8",
            n);
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (auto &functor : functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}