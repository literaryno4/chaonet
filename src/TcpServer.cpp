//
// Created by chao on 2022/3/15.
//

#include "TcpServer.h"

#include <stdio.h>

#include <memory>

#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Logging.h"
#include "SocketsOps.h"

using namespace chaonet;
using namespace muduo;

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAdr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(listenAdr.toHostPort()),
      acceptor_(new Acceptor(loop, listenAdr)),
      threadPool_(new EventLoopThreadPool(loop)),
      started_(false),
      nextConnId_(1) {
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,
                                                  this, std::placeholders::_1,
                                                  std::placeholders::_2));
}

TcpServer::~TcpServer() {}

void TcpServer::setThreadNum(int numThreads) {
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    if (!started_) {
        started_ = true;
        threadPool_->start();
    }

    if (!acceptor_->listening()) {
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
        LOG_INFO << "listening on port: " << name_;
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf, sizeof(buf), "#%d", nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection ["
             << connName << "] from " << peerAddr.toHostPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    auto ioLoop = threadPool_->getNextLoop();
    auto conn = std::make_shared<TcpConnection>(ioLoop, connName, sockfd,
                                                localAddr, peerAddr);
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    //    conn->connectionEstablished();
    ioLoop->runInLoop(std::bind(&TcpConnection::connectionEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());
    assert(n == 1); (void)n;
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectionDestroyed, conn));
}
