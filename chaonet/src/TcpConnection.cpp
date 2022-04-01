//
// Created by chao on 2022/3/15.
//

#include "TcpConnection.h"

#include <unistd.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"
#include "utils/tools.h"

using namespace chaonet;

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name,
                             int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(checkNotNUll(loop)),
      name_(name),
      state_(StateE::kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr) {
    SPDLOG_DEBUG("TcpConnection::constructor[{}],  fd=", name_, sockfd);
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection() {
    SPDLOG_DEBUG("TcpConnection::dtor[{}], fd={}", name_, channel_->fd());
}

void TcpConnection::send(const std::string &message) {
    if (state_ == StateE::kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            loop_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::sendInLoop(const std::string &message) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message.data(), message.size());
        if (nwrote >= 0) {
            if (static_cast<size_t>(nwrote) < message.size()) {
                SPDLOG_TRACE("I am going to write more data");
            } else if (writeCompleteCallback_) {
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                SPDLOG_ERROR("TcpConnection::sendInLoop");
            }
        }
    }
    assert(nwrote >= 0);
    if (static_cast<size_t>(nwrote) < message.size()) {
        outputBuffer_.append(message.data() + nwrote, message.size() - nwrote);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::connectionEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == StateE::kConnecting);
    setState(StateE::kConnected);
    channel_->enableReading();
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectionDestroyed() {
    loop_->assertInLoopThread();
    assert(state_ == StateE::kConnected || state_ == StateE::kDisconnecting);
    setState(StateE::kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());
    loop_->removeChannel(channel_.get());
}

void TcpConnection::shutdown() {
    if (state_ == StateE::kConnected) {
        setState(StateE::kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::setTcpNoDelay(bool on) { socket_->setTcpNoDelay(on); }

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        handleError();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(),
                            outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == StateE::kDisconnecting) {
                    shutdownInLoop();
                }
            } else {
                SPDLOG_TRACE("I am going to write more data");
            }
        } else {
            SPDLOG_ERROR("TcpConnection::handleWrite");
        }
    } else {
        SPDLOG_TRACE("Connection is down, no more writing");
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    SPDLOG_TRACE("TcpConnection::handleClose state = {}",
                 static_cast<int>(state_));
    assert(state_ == StateE::kConnected || state_ == StateE::kDisconnecting);
    channel_->disableAll();
}

void TcpConnection::handleError() {
    char buf[512];
    int err = sockets::getSocketError(channel_->fd());
    SPDLOG_ERROR("TcpConnection::handleError [{}] - SO_ERROR - {} {}", name_, err, strerror_r(err, buf, 512));
}