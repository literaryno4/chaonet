//
// Created by chao on 2022/5/13.
//

#include "HttpServer.h"

#include <spdlog/spdlog.h>
#include <unistd.h>

#include "HttpContext.h"
#include "HttpResponse.h"

using namespace chaonet;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

const int kBufSize = 64*1024;

namespace chaonet {

void defaultHttpCallback(const HttpRequest &, HttpResponse *resp) {
    resp->setStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}
}  // namespace chaonet

HttpServer::HttpServer(EventLoop *loop, const InetAddress &listenAddr,
                       const std::string &name, TcpServer::Option option)
    : server_(loop, listenAddr), httpCallback_(defaultHttpCallback) {
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

void HttpServer::start() {
    SPDLOG_WARN("HttpServer starts listening on {}", server_.name());
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn) {
    if (conn->connected()) {
        conn->setContext(HttpContext());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf,
                           Timestamp receiveTime) {
    HttpContext *context = std::any_cast<HttpContext>(conn->getMutableContext());
    if (!context->parseRequest(buf, receiveTime)) {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    if (context->gotAll()) {
        onRequest(conn, context->request());
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr &conn,
                           const HttpRequest &req) {
    const string &connection = req.getHeader("Connection");
    bool close = connection == "close" ||
                 (req.getVersion() == HttpRequest::Version::kHttp10 &&
                  connection != "Keep-Alive");
    HttpResponse response(close);

    string fileName = req.getFullFileName();
    SPDLOG_DEBUG("filename: {}", fileName);
    FILE* fp = ::fopen(fileName.c_str(), "rb");
    size_t nread = 0;
    if (fp && fileName != req.getFileRoot()) {
        response.setStatusCode(HttpResponse::HttpStatusCode::k200k);
        response.setStatusMessage("OK");
        response.setContentType(req.getFileType());
        response.addHeader("Transfer-Encoding", "chunked");
        response.addHeader("Server", "Chaonet");

        Buffer bufToSend;
        response.appendToBuffer(&bufToSend);
        conn->send(&bufToSend);

        char chunck[kBufSize];
        do {
            nread = ::fread(chunck, 1, sizeof(chunck), fp);
            response.setBody(string(chunck, chunck + nread));
            std::stringstream ss;
            ss << std::hex << nread;
            conn->send(ss.str() + "\r\n" );
            conn->send(string(chunck, chunck + nread) + "\r\n");
        } while (nread > 0);
        conn->send("0\r\n" );
        conn->send("\r\n");
        ::fclose(fp);
    } else {
        httpCallback_(req, &response);
        Buffer bufToSend;
        response.appendToBuffer(&bufToSend);
        conn->send(&bufToSend);
    }

    if (response.closeConnection()) {
        conn->shutdown();
    }
}