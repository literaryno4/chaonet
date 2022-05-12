//
// Created by chao on 2022/4/11.
//

#ifndef CHAONET_TCPCLIENT_H
#define CHAONET_TCPCLIENT_H

#include <mutex>
#include "TcpConnection.h"

namespace chaonet {

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient {
   public:
    TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name = "");
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();
    TcpConnectionPtr connection() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return connection_;
    }

    bool retry();
    void enableRetry() {
        retry_ = true;
    }

   public:
    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback & cb) {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback & cb) {
        writeCompleteCallback_ = cb;
    }

   private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    ConnectorPtr connector_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    bool retry_;
    bool connect_;
    int nextConnId_;
    mutable std::mutex mutex_;
    TcpConnectionPtr connection_;
    std::string name_;
};

}
#endif  // CHAONET_TCPCLIENT_H
