//
// Created by chao on 2022/5/24.
//

#include "TcpServer.h"
#include "EventLoop.h"
#include "Sudoku.h"
#include <spdlog/spdlog.h>

using namespace chaonet;
using std::string;

const int kCells = 81;

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
    size_t len = buf->readableBytes();
    spdlog::debug("readable len {}", len);
    while (len >= kCells + 2) {
        const char* crlf = buf->findCRLF();
        if (crlf) {
            string request(buf->peek(), crlf);
            string id;
            buf->retrieveUntil(crlf + 2);
            auto colon = find(request.begin(), request.end(), ':');
            if (colon != request.end()) {
                id.assign(request.begin(), colon);
                request.erase(request.begin(), colon + 1);
            }
            if (request.size() >= static_cast<size_t>(kCells)) {
                Sudoku sudoku(request);
                sudoku.solveSudoku();
                string result = sudoku.result();
                if (id.empty()) {
                    conn->send(result+"\r\n");
                } else {
                    conn->send(id + request + "\r\n");
                }
            } else {
                conn->send("Bad Request!\r\n");
                conn->shutdown();
            }
        } else {
            break;
        }
    }
}

int main(int argc, const char** argv) {
    spdlog::set_level(spdlog::level::debug);

    int numThread = 0;
    if (argc > 1) {
        numThread = atoi(argv[1]);
    }
    EventLoop loop;
    InetAddress addr(9981);
    TcpServer server(&loop, addr);

    server.setThreadNum(numThread);
    server.setMessageCallback(onMessage);

    server.start();
    loop.loop();
}