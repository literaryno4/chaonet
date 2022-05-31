//
// Created by chao on 22-5-30.
//

#include <spdlog/spdlog.h>

#include "EventLoop.h"
#include "TcpClient.h"
#include "utils/Thread.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class DiscardClient {
   public:
    DiscardClient(chaonet::EventLoop* loop,
                  const chaonet::InetAddress& listenAddr, int size)
        : loop_(loop), client_(loop, listenAddr), message_(size, 'H') {
        client_.setConnectionCallback((std::bind(&DiscardClient::onConnection, this, _1)));
        client_.setMessageCallback((std::bind(&DiscardClient::onMessage, this, _1, _2, _3)));
        client_.setWriteCompleteCallback((std::bind(&DiscardClient::onWriteComplete, this, _1)));
    }

    void connect() { client_.connect(); }

   private:
    void onConnection(const chaonet::TcpConnectionPtr& conn) {
        SPDLOG_TRACE("{} -> {} is {}", conn->peerAddress().toHostPort(),
                     conn->localAddress().toHostPort(),
                     (conn->connected() ? "up" : "down"));
        if (conn->connected()) {
            conn->setTcpNoDelay(true);
            conn->send(message_);
        } else {
            loop_->quit();
        }
    }
    void onMessage(const chaonet::TcpConnectionPtr& conn, chaonet::Buffer* buf,
                   chaonet::Timestamp time) {
        buf->retrieveAll();
    }
    void onWriteComplete(const chaonet::TcpConnectionPtr& conn) {
        SPDLOG_INFO("write complete {}", message_.size());
        if (i == 0) {
            conn->send(message_);
            i = 1;
        }
//        conn->send(message_);
    }
    chaonet::EventLoop* loop_;
    chaonet::TcpClient client_;
    std::string message_;
    int i = 0;
};

int main() {
    SPDLOG_INFO("pid = {}, tid = {}", getpid(), chaonet::getTid());

    chaonet::EventLoop loop;
    chaonet::InetAddress serverAddr("127.0.0.1", 2009);
    int size = 256;

    DiscardClient client(&loop, serverAddr, size);
    client.connect();
    loop.loop();
}
