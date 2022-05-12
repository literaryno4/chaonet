//
// Created by chao on 2022/4/27.
//

#include "EventLoop.h"
#include "TcpServer.h"

int main() {
    chaonet::EventLoop loop;
    chaonet::TcpServer server(&loop, chaonet::InetAddress(9982));
    auto onConnection = [](auto& conn) {
        if (conn->connected()) {
            conn->shutdown();
        }
    };
    server.setConnectionCallback(onConnection);
    server.start();
    loop.loop();
}

