//
// Created by chao on 2022/4/11.
//

#include "TcpClient.h"

#include <spdlog/spdlog.h>

#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "utils/tools.h"

using namespace chaonet;

namespace chaonet {
namespace detail {

void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn) {
    loop->queueInLoop(std::bind(&TcpConnection::connectionDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector) {}

}  // namespace detail
}  // namespace chaonet

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(checkNotNUll(loop)),
      connector_(std::make_shared<Connector>(loop, serverAddr)),
      retry_(false),
      connect_(true),
      nextConnId_(1) {
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
    SPDLOG_INFO("TcpClient::TcpClient - connector");
}

TcpClient::~TcpClient() {
    SPDLOG_INFO("TcpClient::~TcpClient - connector");
    TcpConnectionPtr conn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conn = connection_;
    }
    if (conn) {
        CloseCallback  cb = std::bind(&detail::removeConnection, loop_, std::placeholders::_1);
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
    } else {
        connector_->stop();
        loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    }
}

void TcpClient::connect() {
    SPDLOG_INFO("TcpClient::connect - connecting to {}", connector_->serverAddress().toHostPort());
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect() {
    connect_ = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_) {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    std::stringstream ss;
    ss << peerAddr.toHostPort().c_str() << '#' << nextConnId_;
    std::string connName = ss.str();

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    auto conn = std::make_shared<TcpConnection>(loop_, connName, sockfd, localAddr, peerAddr);
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectionEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());
    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectionDestroyed, conn));
    if (retry_ && connect_) {
        SPDLOG_INFO("TcpClient::connect - Reconnecting to {}", connector_->serverAddress().toHostPort());
        connector_->restart();
    }
}

