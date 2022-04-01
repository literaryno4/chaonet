//
// Created by chao on 2022/3/15.
//

#include <unistd.h>

#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

void newConnection(int sockfd, const chaonet::InetAddress& peerAddr) {
    printf("newConnection(): accepted a new connection from %s\n",
           peerAddr.toHostPort().c_str());
    ::write(sockfd, "How are you?\n", 13);
    chaonet::sockets::close(sockfd);
}

int main() {
    printf("main(): pid = %d\n", getpid());

    chaonet::InetAddress listenAddr(9981);
    chaonet::EventLoop loop;

    chaonet::Acceptor acceptor(&loop, listenAddr);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();

    loop.loop();
}