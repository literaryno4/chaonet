//
// Created by chao on 2022/4/28.
//

#include <spdlog/spdlog.h>
#include <stdio.h>
#include <unistd.h>

#include <atomic>

#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "TcpClient.h"

using namespace chaonet;

using std::string;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class Client;

class Session {
   public:
    Session(EventLoop* loop, const InetAddress& serverAddr, const string& name,
            Client* owner)
        : client_(loop, serverAddr, name),
          owner_(owner),
          bytesRead_(0),
          bytesWritten_(0),
          messagesRead_(0) {
        client_.setConnectionCallback(
            std::bind(&Session::onConnection, this, _1));
        client_.setMessageCallback(
            std::bind(&Session::onMessage, this, _1, _2, _3));
    }

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void start() { client_.connect(); }

    void stop() { client_.disconnect(); }

    int64_t bytesRead() const { return bytesRead_; }

    int64_t messagesRead() const { return messagesRead_; }

   private:
    void onConnection(const TcpConnectionPtr& coon);
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
        ++messagesRead_;
        bytesRead_ += buf->readableBytes();
        bytesWritten_ += buf->readableBytes();
        conn->send(buf);
    }
    TcpClient client_;
    Client* owner_;
    int64_t bytesRead_;
    int64_t bytesWritten_;
    int64_t messagesRead_;
};

class Client {
   public:
    Client(EventLoop* loop, const InetAddress& serverAddr, int blockSize,
           int sessionCount, int timeout, int threadCount)
        : loop_(loop),
          threadPool_(loop),
          sessionCount_(sessionCount),
          timeout_(timeout) {
        loop->runAfter(timeout, std::bind(&Client::handleTimeout, this));
        if (threadCount > 1) {
            threadPool_.setThreadNum(threadCount);
        }
        threadPool_.start();

        for (int i = 0; i < blockSize; ++i) {
            message_.push_back(static_cast<char>(i % 128));
        }

        for (int i = 0; i < sessionCount; ++i) {
            char buf[32];
            snprintf(buf, sizeof(buf), "C%05d", i);
            auto session = std::make_unique<Session>(threadPool_.getNextLoop(),
                                                     serverAddr, buf, this);
            session->start();
            sessions_.emplace_back(std::move(session));
        }
    }

    const string& message() const { return message_; }

    void onConnect() {
        numConnected_.fetch_add(1);
        if (numConnected_ == sessionCount_) {
            SPDLOG_WARN("all connected");
        }
    }

    void onDisconnect(const TcpConnectionPtr& conn) {
        numConnected_.fetch_sub(1);
        if (numConnected_ == 0) {
            SPDLOG_WARN("all disconnected");
            int64_t totalBytesRead = 0;
            int64_t totalMessagesRead = 0;
            for (const auto& session : sessions_) {
                totalBytesRead += session->bytesRead();
                totalMessagesRead += session->messagesRead();
            }
            SPDLOG_WARN("{} total bytes read", totalBytesRead);
            SPDLOG_WARN("{} total messages read", totalMessagesRead);
            SPDLOG_WARN("{} average message size",
                        static_cast<double>(totalBytesRead) /
                            static_cast<double>(totalMessagesRead));
            SPDLOG_WARN(
                "{} MiB/s throughput",
                static_cast<double>(totalBytesRead) / (timeout_ * 1024 * 1024));
            conn->getLoop()->queueInLoop(std::bind(&Client::quit, this));
        }
    }

   private:
    void quit() { loop_->queueInLoop(std::bind(&EventLoop::quit, loop_)); }

    void handleTimeout() {
        SPDLOG_WARN("stop");
        for (auto& session : sessions_) {
            session->stop();
        }
    }

    EventLoop* loop_;
    EventLoopThreadPool threadPool_;
    int sessionCount_;
    int timeout_;
    std::vector<std::unique_ptr<Session>> sessions_;
    string message_;
    std::atomic<int32_t> numConnected_;
};

void Session::onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        conn->setTcpNoDelay(true);
        conn->send(owner_->message());
        owner_->onConnect();
    } else {
        owner_->onDisconnect(conn);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        fprintf(stderr,
                "Usage: client <host_ip> <port> <threads> <blocksize> "
                "<sessions> <time>\n");
    } else {
        SPDLOG_INFO("pid = {}, tid = {}", getpid(), getTid());
        spdlog::set_level(spdlog::level::warn);

        const char* ip = argv[1];
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        int threadCount = atoi(argv[3]);
        int blockSize = atoi(argv[4]);
        int sessionCount = atoi(argv[5]);
        int timeout = atoi(argv[6]);

        EventLoop loop;
        InetAddress serverAddr(ip, port);

        Client client(&loop, serverAddr, blockSize, sessionCount, timeout,
                      threadCount);
        loop.loop();
    }
}