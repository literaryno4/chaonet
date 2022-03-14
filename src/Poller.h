//
// Created by chao on 2022/3/13.
//

#ifndef CHAONET_POLLER_H
#define CHAONET_POLLER_H

#include <map>
#include <vector>

#include "Timestamp.h"

#include "EventLoop.h"

struct pollfd;

namespace chaonet {

class Channel;

class Poller {
   public:
    typedef std::vector<Channel*> ChannelList;

    Poller(EventLoop* loop);
    ~Poller();

    muduo::Timestamp poll(int timeoutMs, ChannelList* activeChannels);
    void updateChannel(Channel* channel);

    void assertInLoopThread() { ownerLoop_->assertInLoopThread();}

   private:
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    typedef std::vector<struct pollfd> PollFdList;
    typedef std::map<int, Channel*> ChannelMap;

    EventLoop* ownerLoop_;
    PollFdList pollfds_;
    ChannelMap channels_;
};

}  // namespace chaonet

#endif  // CHAONET_POLLER_H
