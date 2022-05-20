//
// Created by chao on 2022/5/13.
//

#ifndef CHAONET_HTTPSERVER_H
#define CHAONET_HTTPSERVER_H

#include <functional>

#include "TcpServer.h"

namespace chaonet {
class HttpRequest;
class HttpResponse;

class HttpServer {
   public:
    typedef std::function<void(const HttpRequest&, HttpResponse*)> HttpCallback;
    HttpServer(EventLoop* loop, const InetAddress& listenAddr,
               const std::string& name,
               TcpServer::Option option = TcpServer::Option::kNoReusePort);
    EventLoop* getLoop() const { return server_.getLoop(); }

    void setHttpCallback(const HttpCallback& cb) { httpCallback_ = cb; }

    void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

    void start();

   private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf,
                   Timestamp receiveTime);
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);
    TcpServer server_;
    HttpCallback httpCallback_;
};

}  // namespace chaonet

#endif  // CHAONET_HTTPSERVER_H
