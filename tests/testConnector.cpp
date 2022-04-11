//
// Created by chao on 2022/4/8.
//

#include "Connector.h"
#include "EventLoop.h"

#include <stdio.h>

chaonet::EventLoop* g_loop;

int main () {
    chaonet::EventLoop loop;
    g_loop = &loop;
    chaonet::InetAddress addr("127.0.0.1", 9981);
    auto connector = std::make_shared<chaonet::Connector>(&loop, addr);
    connector->setNewConnectionCallback([](int) {
        printf("connected. \n");
        g_loop->quit();
    });
    connector->start();
    loop.loop();

    return 0;
}