//
// Created by chao on 2022/3/16.
//

#include <spdlog/spdlog.h>
#include <stdio.h>
#include <unistd.h>

#include "EventLoop.h"
#include "TcpServer.h"

void onConnection(const chaonet::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        printf("onConnection(): new connection [%s] from %s\n",
               conn->name().c_str(), conn->peerAddress().toHostPort().c_str());
    } else {
        printf("onConnection(): connection [%s] is down\n",
               conn->name().c_str());
    }
}

void onConnectionSend(const chaonet::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        printf("onConnectionSend(): new connection [%s] from %s\n",
               conn->name().c_str(), conn->peerAddress().toHostPort().c_str());
        ::sleep(5);
        conn->send("yahaha\n");
        conn->send("yahaha");
        conn->shutdown();
    } else {
        printf("onConnection(): connection [%s] is down\n",
               conn->name().c_str());
    }
}

void onMessage(const chaonet::TcpConnectionPtr& conn, chaonet::Buffer* buf,
               chaonet::Timestamp receiveTime) {
    printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
           buf->readableBytes(), conn->name().c_str(),
           receiveTime.toFormattedString().c_str());
    printf("onMessage(): [%s]\n", buf->retrieveAsString().c_str());
}

void onMessageSend(const chaonet::TcpConnectionPtr& conn, chaonet::Buffer* buf,
                   chaonet::Timestamp receiveTime) {
    printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
           buf->readableBytes(), conn->name().c_str(),
           receiveTime.toFormattedString().c_str());
    conn->send(buf->retrieveAsString());
}

void onWriteComplete(const chaonet::TcpConnectionPtr& conn) {
    conn->send("write complete\n");
}

void test1() {
    printf("main(): pid = %d\n", getpid());
    chaonet::InetAddress listenAddr(9981);
    chaonet::EventLoop loop;

    chaonet::TcpServer server(&loop, listenAddr);
    server.setConnectionCallback(onConnection);
    //    server.setConnectionCallback(onConnectionSend);
    //    server.setMessageCallback(onMessage);
    server.setMessageCallback(onMessageSend);
    server.setWriteCompleteCallback(onWriteComplete);
    server.start();

    loop.loop();
}

void test2(int threadNum) {
    printf("main(): pid = %d\n", getpid());
    chaonet::InetAddress listenAddr(9981);
    chaonet::EventLoop loop;

    chaonet::TcpServer server(&loop, listenAddr);
//    server.setConnectionCallback(onConnection);
    server.setConnectionCallback([](const chaonet::TcpConnectionPtr& conn) {
        if (conn->connected()) {
            printf("lambda on Connection\n");
        } else {
            printf("lambda on Connection, conn is down\n");
        }
    });
    server.setMessageCallback(onMessageSend);
    server.setThreadNum(threadNum);
    server.start();

    loop.loop();
}

int main() {
    test2(5);

    return 0;
};
