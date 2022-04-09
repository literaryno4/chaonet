//
// Created by chao on 2022/4/2.
//

#ifndef CHAONET_CONNECTOR_H
#define CHAONET_CONNECTOR_H

#include "InetAddress.h"
#include "TimerId.h"

#include <functional>
#include <memory>

namespace chaonet {

class Channel;
class EventLoop;

class Connector {
   private:
    typedef std::function<void (int sockfd)> NewConnectionCallback;
    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        newConnectionCallback_ = cb;
    }

    void start();
    void restart();
    void stop();

    const InetAddress& serverAddress() const {
        return serverAddr_;
    }

   private:
    enum class States { kDisconnected, kConnecting, kConnected};
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    void setStates(States s) { state_ = s;}
    void startInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    EventLoop* loop_;
    InetAddress serverAddr_;
    bool connect_;
    States state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDelayMs_;
    TimerId timerId_;
};

typedef std::shared_ptr<Connector> ConnectorPtr;

}
#endif  // CHAONET_CONNECTOR_H
