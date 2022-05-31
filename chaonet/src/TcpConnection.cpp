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

void chaonet::defaultConnectionCallback(const TcpConnectionPtr &conn) {
    SPDLOG_TRACE("{} -> {} is {}", conn->localAddress().toHostPort(),
                 conn->peerAddress().toHostPort(),
                 conn->connected() ? "UP" : "DOWN");
}

void chaonet::defaultMessageCallback(const TcpConnectionPtr &, Buffer *buf,
                                     Timestamp) {
    buf->retrieveAll();
}

using namespace chaonet;

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name,
                             int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(checkNotNUll(loop)),
      name_(name),
      state_(StateE::kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback) {
    spdlog::debug("TcpConnection::constructor[{}],  fd={}", name_, sockfd);
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection() {
    spdlog::debug("TcpConnection::dtor[{}], fd={}", name_, channel_->fd());
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

void TcpConnection::send(const char *message, int len) {
    send(std::string(message, message + len));
}

void TcpConnection::send(Buffer *buf) {
    if (state_ == StateE::kConnected) {
        std::string msg = buf->retrieveAsString();
        if (loop_->isInLoopThread()) {
            sendInLoop(msg);
        } else {
            auto fp = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, msg));
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
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + (message.size() - nwrote) >= highWaterMark_ &&
            oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_,
                                         shared_from_this(),
                                         oldLen + (message.size() - nwrote)));
        }
        outputBuffer_.append(message.data() + nwrote, message.size() - nwrote);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::startRead() {
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop() {
    loop_->assertInLoopThread();
    if (!reading_ || !channel_->isReading()) {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead() {
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop() {
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading()) {
        channel_->disableReading();
        reading_ = false;
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
    if (state_ == StateE::kConnected) {
        //        assert(state_ == StateE::kConnected || state_ ==
        //        StateE::kDisconnecting);
        setState(StateE::kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
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
    spdlog::debug("TcpConnection::handleClose state = {}",
                  static_cast<int>(state_));
    assert((state_ == StateE::kConnected) ||
           (state_ == StateE::kDisconnecting));
    setState(StateE::kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    closeCallback_(guardThis);
}

void TcpConnection::handleError() {
    char buf[512];
    int err = sockets::getSocketError(channel_->fd());
    SPDLOG_ERROR("TcpConnection::handleError [{}] - SO_ERROR - {} {}", name_,
                 err, strerror_r(err, buf, 512));
}