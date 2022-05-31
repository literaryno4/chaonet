//
// Created by chao on 22-5-31.
//

#include "tunnel.h"
#include <stdio.h>
#include <netdb.h>

using namespace chaonet;

EventLoop* g_eventLoop;
std::map<std::string, TunnelPtr> g_tunnels;

void onServerConnection(const TcpConnectionPtr& conn) {
    SPDLOG_INFO("{} is {}", conn->name(), (conn->connected() ? "up" : "down"));
    if (conn->connected()) {
        conn->setTcpNoDelay(true);
    } else {
        auto it = g_tunnels.find(conn->name());
        if (it != g_tunnels.end()) {
            it->second->disconnect();
            g_tunnels.erase(it);
        }
    }
}

void onServerMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
    spdlog::debug("{} {} bytes readable", conn->name(), buf->readableBytes());
    if (g_tunnels.find(conn->name()) == g_tunnels.end()) {
        if (buf->readableBytes() > 128) {
            conn->shutdown();
        } else if (buf->readableBytes() > 8) {
            const char* begin = buf->peek() + 8;
            const char* end = buf->peek() + buf->readableBytes();
            const char* where = std::find(begin, end, '\0');
            if (where != end) {
                char ver = buf->peek()[0];
                char cmd = buf->peek()[1];
                const void* port = buf->peek() + 2;
                const void* ip = buf->peek() + 4;
                SPDLOG_INFO("ip {}, port {}", ip, port);

                sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = *static_cast<const in_port_t*>(port);
                addr.sin_addr.s_addr = *static_cast<const uint32_t*>(ip);

                bool socks4a = sockets::networkToHost32(addr.sin_addr.s_addr) < 256;
                bool okay = false;
                if (socks4a) {
                    const char* endOfHostName = std::find(where + 1, end, '\0');
                    if (endOfHostName != end)  {
                        std::string hostname = where + 1;
                        where = endOfHostName;
                        SPDLOG_INFO("Socks4a host name {}", hostname);
                        InetAddress tmp;
                        if (InetAddress::resolve(hostname, &tmp)) {
                            addr.sin_addr.s_addr = tmp.ipv4NetEndian();
                            okay = true;
                        }
                    } else {
                        return;
                    }
                } else {
                    okay = true;
                }

                InetAddress serverAddr(addr);
                if (ver == 4 && cmd == 1 && okay) {
                    auto tunnel = std::make_shared<Tunnel>(g_eventLoop, serverAddr, conn);
                    tunnel->setup();
                    tunnel->connect();
                    g_tunnels[conn->name()] = tunnel;
                    buf->retrieveUntil(where+1);
                    char response[] = "\000\x5aUVWXYZ";
                    memcpy(response+2, &addr.sin_port, 2);
                    memcpy(response+4, &addr.sin_addr.s_addr, 4);
                    conn->send(response, 8);
                } else {
                    char response[] = "\000\x5bUVWXYZ";
                    conn->send(response, 8);
                    conn->shutdown();
                }
            }
        }
    } else if (conn->getContext().has_value()) {
        const TcpConnectionPtr& clientConn = std::any_cast<const TcpConnectionPtr&>(conn->getContext());
        clientConn->send(buf);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <listen_port>\n", argv[0]);
    } else {
        uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        InetAddress listenAddr(port);

        EventLoop loop;
        g_eventLoop = &loop;

        TcpServer server(&loop, listenAddr, "Socks4");

        server.setConnectionCallback(onServerConnection);
        server.setMessageCallback(onServerMessage);

        server.start();
        loop.loop();
    }
}