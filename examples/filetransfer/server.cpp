//
// Created by chao on 2022/5/24.
//

#include "EventLoop.h"
#include "TcpServer.h"

#include <stdio.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

using namespace chaonet;

const int kBufSize = 64*1024;
const char* g_file = nullptr;
typedef  std::shared_ptr<FILE> FilePtr;

void transFile(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        FILE* fp = ::fopen(g_file, "rb");
        if (fp) {
            FilePtr ctx(fp, ::fclose);
            conn->setContext(ctx);
            char buf[kBufSize];
            size_t nread = ::fread(buf, 1, sizeof(buf), fp);
            conn->send(buf, static_cast<int>(nread));
        } else {
            conn->shutdown();
            SPDLOG_INFO("FileServer - no such file!");
        }
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
    if (conn->connected()) {
        const char* crlf = buf->findCRLF();
        if (crlf) {
            std::string filename(buf->peek(), crlf);
            buf->retrieveUntil(crlf + 2);
            g_file = filename.c_str();
        }
        if (g_file) {
            SPDLOG_INFO("file name {}", g_file);
            transFile(conn);
        }
    } else {
        conn->shutdown();
    }
}

void onWriteComplete(const TcpConnectionPtr& conn) {
    const FilePtr& fp = std::any_cast<const FilePtr>(conn->getContext());
    char buf[kBufSize];
    size_t nread = ::fread(buf, 1, sizeof(buf), fp.get());
    if (nread > 0) {
        conn->send(buf, static_cast<int>(nread));
    } else {
        conn->shutdown();
        SPDLOG_INFO("FileServer - done");
    }
}

int main(int argc, char* argv[]) {
    EventLoop loop;
    InetAddress addr(2022);
    TcpServer server(&loop, addr);
    server.setMessageCallback(onMessage);
    server.setWriteCompleteCallback(onWriteComplete);
    server.start();
    loop.loop();
}
