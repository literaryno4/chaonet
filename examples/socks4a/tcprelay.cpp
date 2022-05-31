//
// Created by chao on 22-5-31.
//
#include <malloc.h>
#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

#include "tunnel.h"
#include "utils/Thread.h"

using namespace chaonet;

EventLoop* g_eventLoop;
InetAddress* g_serverAddr;
std::map<std::string, TunnelPtr> g_tunnels;

void onServerConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        conn->setTcpNoDelay(true);
        conn->stopRead();
        auto tunnel =
            std::make_shared<Tunnel>(g_eventLoop, *g_serverAddr, conn);
        tunnel->setup();
        tunnel->connect();
        g_tunnels[conn->name()] = tunnel;
    } else {
        assert(g_tunnels.find(conn->name()) != g_tunnels.end());
        g_tunnels[conn->name()]->disconnect();
        g_tunnels.erase(conn->name());
    }
}

void onServerMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
    if (conn->getContext().has_value()) {
        const TcpConnectionPtr& clientConn = std::any_cast<const TcpConnectionPtr&>(conn->getContext());
        clientConn->send(buf);
    }
}

void memstat() {
    malloc_stats();
}

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::trace);
    SPDLOG_LEVEL_TRACE;
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <host_ip> <port> <listen_port>\n", argv[0]);
    } else {
        SPDLOG_INFO("pid = {}, tid = {}", getpid(), chaonet::getTid());
        {
            size_t kOneMB = 1024 * 1024;
            rlimit rl = {256*kOneMB, 256*kOneMB};
            setrlimit(RLIMIT_AS, &rl);
        }

        const char* ip = argv[1];
        uint16_t  port = static_cast<uint16_t>(atoi(argv[2]));
        InetAddress serverAddr(ip, port);
        g_serverAddr = &serverAddr;

        uint16_t  acceptPort = static_cast<uint16_t>(atoi(argv[3]));
        InetAddress listenAddr(acceptPort);

        EventLoop loop;
        g_eventLoop = &loop;
        loop.runEvery(3, memstat);
        TcpServer server(&loop, listenAddr, "TcpRelay");

        server.setConnectionCallback(onServerConnection);
        server.setMessageCallback(onServerMessage);

        server.start();

        loop.loop();
    }
}