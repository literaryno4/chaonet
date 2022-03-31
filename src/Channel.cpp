//
// Created by chao on 2022/3/13.
//

#include "Channel.h"
#include "EventLoop.h"
#include "Logging.h"

#include <sstream>

#include <assert.h>
#include <sys/poll.h>

using namespace chaonet;
using namespace muduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1) {}

void Channel::update() {
    loop_->updateChannel(this);
}

Channel::~Channel() {
    assert(!eventHandling);

}

void Channel::handleEvent(Timestamp receiveTime) {
    eventHandling = true;
    if (revents_ & POLLNVAL) {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        LOG_WARN << " Channel::handleEvent() POLLHUP";
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) {
            errorCallback_();
        }
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    }
    if (revents_ & POLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
    eventHandling = false;
}