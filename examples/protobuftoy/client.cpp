//
// Created by chao on 2022/5/27.
//

#include "EventLoop.h"
#include "TcpServer.h"
#include "person.pb.h"

#include <string>

using namespace chaonet;

#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpClient.h"
#include "person.pb.h"

int main() {
    Person* p1 = new Person;
    p1->set_name("chao");
    p1->set_id(173);
    std::stringstream ss;
    std::string s;
    p1->SerializeToString(&s);


    chaonet::EventLoop loop;
    chaonet::InetAddress serverAddr("127.0.0.1", 8880);
    chaonet::TcpClient client(&loop, serverAddr);

    client.setConnectionCallback([&](const chaonet::TcpConnectionPtr& conn) {
        if (conn->connected()) {
            printf("onConnection(): new connection [%s] from %s\n",
                   conn->name().c_str(),
                   conn->peerAddress().toHostPort().c_str());
            conn->send(s);
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
