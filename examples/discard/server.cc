//
// Created by chao on 22-5-30.
//

#include <spdlog/spdlog.h>

#include <atomic>

#include "EventLoop.h"
#include "TcpServer.h"
#include "utils/Timestamp.h"
#include "utils/Thread.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

int numThreads = 0;

class DiscardServer {
   public:
    DiscardServer(chaonet::EventLoop* loop,
                  const chaonet::InetAddress& listenAddr)
        : server_(loop, listenAddr),
          oldCounter_(0),
          startTime_(chaonet::Timestamp::now()) {
        server_.setConnectionCallback(
            std::bind(&DiscardServer::onConnection, this, _1));
        server_.setMessageCallback(
            std::bind(&DiscardServer::onMessage, this, _1, _2, _3));
        server_.setThreadNum(numThreads);
        loop->runEvery(3.0, std::bind(&DiscardServer::printThroughput, this));
    }
    void start() {
        SPDLOG_INFO("starting {} threads.", numThreads);
        server_.start();
    }

   private:
    void onConnection(const chaonet::TcpConnectionPtr& conn) {
        SPDLOG_TRACE("{} -> {} is {}", conn->peerAddress().toHostPort(),
                     conn->localAddress().toHostPort(),
                     (conn->connected() ? "up" : "down"));
    }
    void onMessage(const chaonet::TcpConnectionPtr& conn, chaonet::Buffer* buf,
                   chaonet::Timestamp) {
        size_t len = buf->readableBytes();
        transferred_.fetch_add(len);
        receivedMessages_.fetch_add(len);
        buf->retrieveAll();
    }
    void printThroughput() {
        chaonet::Timestamp endTime = chaonet::Timestamp::now();
        int64_t newCounter = transferred_.load();
        int64_t bytes = newCounter - oldCounter_;
        int64_t msgs = receivedMessages_.load();
        receivedMessages_.store(0);
        double  time = chaonet::timeDifference(endTime, startTime_);
        printf("%4.3f Mib/s %4.3f ki Msgs/s %6.2f bytes per msg\n",
               static_cast<double>(bytes)/time/1024/1024,
               static_cast<double>(msgs)/time/1024,
               static_cast<double>(bytes)/static_cast<double>(msgs));
        oldCounter_ = newCounter;
        startTime_ = endTime;
    }

   private:
    chaonet::TcpServer server_;
    std::atomic<int64_t> transferred_;
    std::atomic<int64_t> receivedMessages_;
    int64_t oldCounter_;
    chaonet::Timestamp startTime_;
};

int main() {
    SPDLOG_INFO("pid = {}, tid = {}", getpid(), chaonet::getTid());

    chaonet::EventLoop loop;
    chaonet::InetAddress listenAddr(2009);
    DiscardServer server(&loop, listenAddr);

    server.start();
    loop.loop();
}
