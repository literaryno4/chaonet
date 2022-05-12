//
// Created by chao on 2022/4/27.
//

// you add user info in UserMap,  then you can get this in another client.

#include <map>

#include "EventLoop.h"
#include "TcpServer.h"

using std::string;
typedef std::map<string, string> UserMap;
UserMap users;

string getUser(const string& user) {
    string result;
    auto it = users.find(user);
    if (it != users.end()) {
        result = it->second;
    } else {
        // user info is dummy, just for test
        users[user] = "yahaha!";
        result = "add new user, yahaha!";
    }
    return result;
}

void onMessage(const chaonet::TcpConnectionPtr& conn, chaonet::Buffer* buf,
               chaonet::Timestamp receiveTime) {
    const char* crlf = buf->findCRLF();
    if (crlf) {
        string user(buf->peek(), crlf);
        conn->send(getUser(user) + "\r\n");
        buf->retrieveUntil(crlf + 2);
        conn->shutdown();
    }
}

int main() {
    chaonet::EventLoop loop;
    chaonet::TcpServer server(&loop, chaonet::InetAddress(9982));
    server.setMessageCallback(onMessage);
    server.start();
    loop.loop();
}