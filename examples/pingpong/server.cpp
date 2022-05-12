//
// Created by chao on 2022/4/28.
//

#include "EventLoop.h"
#include "TcpServer.h"
#include "utils/Thread.h"

#include <spdlog/spdlog.h>
#include <stdio.h>
#include <unistd.h>

using namespace chaonet;

int main(int argc, const char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: server <address> <port> <threads>\n");
    } else {
        SPDLOG_INFO("pid = {}, tid = {}", getpid(), getTid());
//        spdlog::set_level(spdlog::level::warn);

        const char* ip = argv[1];
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        InetAddress listenAddr(ip, port);
        int threadCount = atoi(argv[3]);

        EventLoop loop;

        TcpServer server(&loop, listenAddr);

        server.setConnectionCallback([](const TcpConnectionPtr& conn){
            if (conn->connected()) {
                conn->setTcpNoDelay(true);
            }
        });
        server.setMessageCallback([](const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
//            std::cout << "received: " << buf->retrieveAsString() << std::endl;
            conn->send(buf->retrieveAsString());
        });

        if (threadCount > 1) {
            server.setThreadNum(threadCount);
        }

        server.start();

        loop.loop();
    }
}