//
// Created by chao on 2022/4/2.
//

#include "Connector.h"

#include <spdlog/spdlog.h>
#include <string.h>
#include <errno.h>

#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

using namespace chaonet;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(States::kDisconnected),
      retryDelayMs_(kInitRetryDelayMs) {
    SPDLOG_DEBUG("ctor[{}]", this);
}

Connector::~Connector() {
    SPDLOG_DEBUG("dtor[{}]", this);
    loop_->cancel(timerId_);
    assert(!channel_);
}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop() {
    loop_->assertInLoopThread();
    assert(state_ == States::kDisconnected);
    if (connect_) {
        connect();
    } else {
        SPDLOG_DEBUG("do not connect");
    }
}

void Connector::connect() {
    int sockfd = sockets::createNonblockingOrDie();
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNABORTED:
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            SPDLOG_ERROR("connection error in Connector::startInLoop {}",
                         savedErrno);
            sockets::close(sockfd);
            break;

        default:
            SPDLOG_ERROR("Unexpected error in Connector::startInLoop {}",
                         savedErrno);
            sockets::close(sockfd);
            break;
    }
}

void Connector::restart() {
    loop_->assertInLoopThread();
    setStates(States::kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop() {
    connect_ = false;
    loop_->cancel(timerId_);
}

void Connector::connecting(int sockfd) {
    setStates(States::kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&Connector::handleError, this));
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    loop_->removeChannel(channel_.get());
    int sockfd = channel_->fd();
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel() { channel_.reset(); }

void Connector::handleWrite() {
    SPDLOG_TRACE("Connector::handleWrite {}", state_);
    if (state_ == States::kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if (err) {
            SPDLOG_WARN("Connector::handleWrite - So_ERROR = {} {}", err,
                        strerror(err));
            retry(sockfd);
        } else if (sockets::isSelfConnect(sockfd)) {
            SPDLOG_WARN("Connector::handleWrite - self connect");
            retry(sockfd);
        } else {
            setStates(States::kConnected);
            if (connect_) {
                newConnectionCallback_(sockfd);
            } else {
                sockets::close(sockfd);
            }
        }
    } else {
        assert(state_ == States::kDisconnected);
    }
}

void Connector::handleError() {
    SPDLOG_ERROR("Connector::handleError");
    assert(state_ == States::kConnecting);

    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    SPDLOG_TRACE("SO_ERROR = {} {}", err, strerror_tl(err));
    retry(sockfd);
}

void Connector::retry(int sockfd) {
    sockets::close(sockfd);
    setStates(States::kDisconnected);
    if (connect_) {
        SPDLOG_INFO(
            "Connector::retry - Retry connecting to {} in {} milliseconds. ",
            serverAddr_.toHostPort(), retryDelayMs_);
        timerId_ = loop_->runAfter(retryDelayMs_ / 1000.0,
                                   std::bind(&Connector::startInLoop, this));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    } else {
        SPDLOG_DEBUG("do not connect");
    }
}
