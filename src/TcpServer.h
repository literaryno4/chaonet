//
// Created by chao on 2022/3/15.
//

#ifndef CHAONET_TCPSERVER_H
#define CHAONET_TCPSERVER_H

#include "Callbacks.h"
#include "TcpConnection.h"

#include <map>

namespace chaonet {

class Acceptor;
class EventLoop;

class TcpServer {
   public:
    TcpServer(EventLoop* Loop, const InetAddress& listenAdr);
    ~TcpServer();

    void start();

    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }

   private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);

    typedef std::map<std::string , TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    bool started_;
    int nextConnId_;
    ConnectionMap connections_;
};

}

#endif  // CHAONET_TCPSERVER_H
