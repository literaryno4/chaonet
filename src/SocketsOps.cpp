//
// Created by chao on 2022/3/15.
//

#include "SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Logging.h"
#include "Types.h"

using namespace chaonet;
using namespace muduo;

typedef struct sockaddr SA;

const SA* sockaddr_cast(const struct sockaddr_in* addr) {
    return static_cast<const SA*>(implicit_cast<const void*>(addr));
}

SA* sockaddr_cast(struct sockaddr_in* addr) {
    return static_cast<SA*>(implicit_cast<void*>(addr));
}

void setNonBlockAndCloseOnExec(int sockfd) {
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, 0);

    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
}

int sockets::createNonblockingOrDie() {
#if VALGRIND
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }
#else
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                          IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }
#endif
    return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr_in& addr) {
    int ret = ::bind(sockfd, sockaddr_cast(&addr), sizeof(addr));
    if (ret < 0) {
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

void sockets::listenOrDie(int sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

int sockets::accept(int sockfd, struct sockaddr_in* addr) {
    socklen_t addrlen = sizeof(*addr);
#if VALGRIND
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if (connfd < 0) {
        int savedErrno = errno;
        LOG_SYSERR << "Socket::accept";
        switch (savedErrno) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EMFILE:
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                LOG_FATAL << "unexpected error of ::accept " << savedErrno;
                break;
            default:
                LOG_FATAL << "unknown error of ::accept" << savedErrno;
                break;
        }
    }
    return connfd;
}

void sockets::close(int sockfd) {
    if (::close(sockfd) < 0) {
        LOG_SYSERR << "sockets::close";
    }
}

void sockets::toHostPort(char* buf, size_t size,
                         const struct sockaddr_in& addr) {
    char host[INET_ADDRSTRLEN] = "INVALID";
    ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof(host));
    uint16_t port = sockets::networkToHost16(addr.sin_port);
    snprintf(buf, size, "%s:%u", host, port);
}

void sockets::fromHostPort(const char* ip, uint16_t port,
                           struct sockaddr_in* addr) {
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr)) {
        LOG_SYSERR << "sockets::fromHostPort";
    }
}