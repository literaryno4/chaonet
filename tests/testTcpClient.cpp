//
// Created by chao on 2022/4/12.
//

#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpClient.h"

int main() {
    chaonet::EventLoop loop;
    chaonet::InetAddress serverAddr("127.0.0.1", 9981);
    chaonet::TcpClient client(&loop, serverAddr);

    client.setConnectionCallback([](const chaonet::TcpConnectionPtr& conn) {
        if (conn->connected()) {
            printf("onConnection(): new connection [%s] from %s\n",
                   conn->name().c_str(),
                   conn->peerAddress().toHostPort().c_str());
            conn->send("yahaha");
        } else {
            printf("onConnection(): connection [%s] is down\n",
                   conn->name().c_str());
        }
    });

    client.setMessageCallback([](const chaonet::TcpConnectionPtr& conn,
                                 chaonet::Buffer* buf,
                                 chaonet::Timestamp receiveTime) {
        printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
               buf->readableBytes(), conn->name().c_str(),
               receiveTime.toFormattedString().c_str());
        printf("onMessage(): [%s]\n", buf->retrieveAsString().c_str());
    });
    client.enableRetry();
    client.connect();
    loop.loop();
}