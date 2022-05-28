//
// Created by chao on 2022/5/27.
//

#include "EventLoop.h"
#include "TcpServer.h"
#include "person.pb.h"

#include <string>

using namespace chaonet;

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
    if (conn->connected()) {
        std::string message = buf->retrieveAsString();
        Person* p1 = new Person;
        p1->ParseFromString(message);
        std::cout << p1->name() << ' ' << p1->id() <<'\n';
    }
}

int main() {
    EventLoop loop;
    InetAddress addr(8880);
    TcpServer server(&loop, addr);

    server.setMessageCallback(onMessage);

    server.start();
    loop.loop();
}