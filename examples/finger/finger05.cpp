//
// Created by chao on 2022/4/27.
//

#include "EventLoop.h"
#include "TcpServer.h"

int main() {
    chaonet::EventLoop loop;
    chaonet::TcpServer server(&loop, chaonet::InetAddress(1079));
    auto onMessage = [](const chaonet::TcpConnectionPtr& conn, chaonet::Buffer* buf, chaonet::Timestamp receiveTime) {
        if (buf->findCRLF()) {
            conn->send("No such user\r\n");
            conn->shutdown();
        }
    };
    server.setMessageCallback(onMessage);
    server.start();
    loop.loop();
}