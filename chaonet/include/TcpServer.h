//
// Created by chao on 2022/3/15.
//

#ifndef CHAONET_TCPSERVER_H
#define CHAONET_TCPSERVER_H

#include "TcpConnection.h"

#include <map>

namespace chaonet {

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer {
   public:

    enum class Option {
        kNoReusePort,
        kReusePort,
    };
    TcpServer(EventLoop* Loop, const InetAddress& listenAdr, const std::string& name = "");
    ~TcpServer();

    void start();

    EventLoop* getLoop() const {
        return loop_;
    }

    const std::string& name() const {
        return name_;
    }

    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }

    // - 0 means all I/O in loop's thread, no thread will be created;
    // - 1 means all I/O in another thread;
    // - N means a thread pool with N threads;
    void setThreadNum(int numThreads);

   private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    typedef std::map<std::string , TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    bool started_;
    int nextConnId_;
    ConnectionMap connections_;
};

}

#endif  // CHAONET_TCPSERVER_H
