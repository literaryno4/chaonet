//
// Created by chao on 2022/4/12.
//

#include "EPoller.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>

#include "Channel.h"

using namespace chaonet;

static_assert(EPOLLIN == POLLIN, "");
static_assert(EPOLLPRI == POLLPRI, "");
static_assert(EPOLLOUT == POLLOUT, "");
static_assert(EPOLLRDHUP == POLLRDHUP, "");
static_assert(EPOLLERR == POLLERR, "");
static_assert(EPOLLHUP == POLLHUP, "");

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}  // namespace

EPoller::EPoller(EventLoop *loop)
    : ownerLoop_(loop),
      epollfd_(::epoll_create(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        SPDLOG_ERROR("EPoller::EPoller()");
    }
}

EPoller::~EPoller() { ::close(epollfd_); }

Timestamp EPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    int numEvent = ::epoll_wait(epollfd_, &*events_.begin(),
                                static_cast<int>(events_.size()), timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvent > 0) {
        SPDLOG_TRACE("{} events happened", numEvent);
        fillActiveChannels(numEvent, activeChannels);
        if (static_cast<size_t>(numEvent) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvent == 0) {
        SPDLOG_TRACE("nothing happened");
    } else {
        SPDLOG_ERROR("EPoller::poll()");
    }
    return now;
}

void EPoller::fillActiveChannels(int numEvents,
                                 ChannelList *activeChannels) const {
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i) {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        int fd = channel->fd();
        auto it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EPoller::updateChannel(Channel *channel) {
    assertInLoopThread();
    SPDLOG_TRACE("fd = {} events = {}", channel->fd(), channel->events());
    const int index = channel->index();
    int fd = channel->fd();
    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        } else {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPoller::removeChannel(Channel *channel) {
    assertInLoopThread();
    int fd = channel->fd();
    SPDLOG_TRACE("fd = {}", fd);
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    assert(n == 1);
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPoller::update(int operation, Channel *channel) {
    struct epoll_event event;
    bzero(&event, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        SPDLOG_ERROR("epoll_ctl op= {} fd = {}", operation, fd);
    }
}