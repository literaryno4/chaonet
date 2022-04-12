//
// Created by chao on 2022/4/12.
//

#ifndef CHAONET_EPOLLER_H
#define CHAONET_EPOLLER_H

#include <map>
#include <vector>

#include "utils/Timestamp.h"
#include "EventLoop.h"

struct epoll_event;

namespace chaonet {

class Channel;

class EPoller {
   public:
    typedef std::vector<Channel*> ChannelList;
    explicit EPoller(EventLoop* loop);
    EPoller(const EPoller&) = delete;
    EPoller& operator=(const EPoller&) = delete;
    ~EPoller();

    Timestamp poll(int timeoutMs, ChannelList* activeChannels);

    void updateChannel(Channel* channel);

    void removeChannel(Channel* channel);

    void assertInLoopThread() {
        ownerLoop_->assertInLoopThread();
    }

   private:
    static const int kInitEventListSize = 16;
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);
    typedef std::vector<struct epoll_event> EventList;
    typedef std::map<int, Channel*> ChannelMap;

    EventLoop* ownerLoop_;
    int epollfd_;
    EventList  events_;
    ChannelMap channels_;
};

}

#endif  // CHAONET_EPOLLER_H
