//
// Created by chao on 2022/4/27.
//

#include "EventLoop.h"
#include "TcpServer.h"

int main() {
    chaonet::EventLoop loop;
    chaonet::TcpServer server(&loop, chaonet::InetAddress(1079));
    server.start();
    loop.loop();
}
