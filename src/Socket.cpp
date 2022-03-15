//
// Created by chao on 2022/3/15.
//

#include "Socket.h"

#include <netinet/in.h>
#include <strings.h>

#include "InetAddress.h"
#include "SocketsOps.h"

using namespace chaonet;

Socket::~Socket() { sockets::close(sockfd_); }

void Socket::bindAddress(const InetAddress &addr) {
    sockets::bindOrDie(sockfd_, addr.getSockAddrInet());
}

void Socket::listen() { sockets::listenOrDie(sockfd_); }

int Socket::accept(InetAddress *peeraddr) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    int connfd = sockets::accept(sockfd_, &addr);
    if (connfd >= 0) {
        peeraddr->setSockAddrInet(addr);
    }
    return connfd;
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}
