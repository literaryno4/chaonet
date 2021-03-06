//
// Created by chao on 2022/3/15.
//

#ifndef CHAONET_TCPCONNECTION_H
#define CHAONET_TCPCONNECTION_H

#include <string>
#include <any>

#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"

namespace chaonet {

class Channel;
class EventLoop;
class Socket;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
   public:
//    typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;

    TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                  const InetAddress& localAddr, const InetAddress& peerAddr);
    TcpConnection(const TcpConnection&) = delete;
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() { return localAddr_; }
    const InetAddress& peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == StateE::kConnected; }

    void setContext(const std::any& context) {
        context_ = context;
    }

    const std::any& getContext() const {
        return context_;
    }

    std::any* getMutableContext() {
        return &context_;
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
    void setHighWaterMarkCallback(const HighWaterMarkCallback & cb, size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

    Buffer* inputBuffer() {
        return &inputBuffer_;
    }
    Buffer* outputBuffer() {
        return &outputBuffer_;
    }
    bool isReading() const {
        return reading_;
    }

    void connectionEstablished();
    void connectionDestroyed();

    void send(const std::string& message);
    void send(const char* message, int len);
    void send(Buffer* buf);
    void startRead();
    void stopRead();
    void shutdown();
    void setTcpNoDelay(bool on);

   private:
    enum class StateE {
        kConnecting = 0,
        kConnected = 1,
        kDisconnecting = 2,
        kDisconnected = 3
    };

    void setState(StateE s) { state_ = s; }
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const std::string& message);
    void startReadInLoop();
    void stopReadInLoop();
    void shutdownInLoop();

    EventLoop* loop_;
    std::string name_;
    StateE state_;
    bool reading_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

    // C++17
    std::any context_;
};

}  // namespace chaonet

#endif  // CHAONET_TCPCONNECTION_H
