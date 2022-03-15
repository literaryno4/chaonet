//
// Created by chao on 2022/3/15.
//

#ifndef CHAONET_ACCEPTOR_H
#define CHAONET_ACCEPTOR_H

#include <functional>
#include "Channel.h"
#include "Socket.h"

namespace chaonet {

class EventLoop;
class InetAddress;

class Acceptor {
   public:
    typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr);
    Acceptor(const Acceptor&) = delete;

    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        newConnectionCallback_ = cb;
    }

    bool listening() const { return listening_;}
    void listen();

   private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
};

}

#endif  // CHAONET_ACCEPTOR_H
