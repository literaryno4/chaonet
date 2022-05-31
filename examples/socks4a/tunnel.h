//
// Created by chao on 22-5-31.
//

#ifndef CHAONET_TUNNEL_H
#define CHAONET_TUNNEL_H

#include <spdlog/spdlog.h>

#include <functional>

#include "EventLoop.h"
#include "TcpClient.h"
#include "TcpServer.h"

class Tunnel : public std::enable_shared_from_this<Tunnel> {
   public:
    Tunnel(chaonet::EventLoop* loop, const chaonet::InetAddress& serverAddr,
           const chaonet::TcpConnectionPtr& serverConn)
        : client_(loop, serverAddr, serverConn->name()),
          serverConn_(serverConn) {
        SPDLOG_INFO("Tunnel {} <-> {}", serverConn->peerAddress().toHostPort(),
                    serverAddr.toHostPort());
    }
    ~Tunnel() { SPDLOG_INFO("~Tunnel"); }

    void setup() {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        client_.setConnectionCallback(
            std::bind(&Tunnel::onClientConnection, shared_from_this(), _1));
        client_.setMessageCallback(std::bind(&Tunnel::onClientMessage,
                                             shared_from_this(), _1, _2, _3));
        serverConn_->setHighWaterMarkCallback(
            std::bind(&Tunnel::onHighWaterMarkWeak,
                      std::weak_ptr<Tunnel>(shared_from_this()),
                      ServerClient::kServer, _1, _2),
            1024 * 1024);
    }
    void connect() { client_.connect(); }
    void disconnect() { client_.disconnect(); }

   private:
    void teardown() {
        client_.setConnectionCallback(chaonet::defaultConnectionCallback);
        client_.setMessageCallback(chaonet::defaultMessageCallback);
        if (serverConn_) {
            serverConn_->setContext(std::any());
            serverConn_->shutdown();
        }
        clientConn_.reset();
    }
    void onClientConnection(const chaonet::TcpConnectionPtr& conn) {
        using std::placeholders::_1;
        using std::placeholders::_2;
        if (conn->connected()) {
            conn->setTcpNoDelay(true);
            conn->setHighWaterMarkCallback(
                std::bind(&Tunnel::onHighWaterMarkWeak,
                          std::weak_ptr<Tunnel>(shared_from_this()),
                          ServerClient::kClient, _1, _2),
                1024 * 1024);
            serverConn_->setContext(conn);
            serverConn_->startRead();
            clientConn_ = conn;
            if (serverConn_->inputBuffer()->readableBytes() > 0) {
                conn->send(serverConn_->inputBuffer());
            }
        } else {
            teardown();
        }
    }

    void onClientMessage(const chaonet::TcpConnectionPtr& conn,
                         chaonet::Buffer* buf, chaonet::Timestamp) {
        SPDLOG_DEBUG("{} {}", conn->name(), buf->readableBytes());
        if (serverConn_) {
            serverConn_->send(buf);
        } else {
            buf->retrieveAll();
            abort();
        }
    }
    enum class ServerClient {
        kServer,
        kClient,
    };

    void onHighWaterMark(ServerClient which,
                         const chaonet::TcpConnectionPtr& conn,
                         size_t bytesToSent) {
        using std::placeholders::_1;

        SPDLOG_INFO("{} onHighWaterMark {} bytes {}",
                    (which == ServerClient::kServer ? "server" : "client"),
                    conn->name(), bytesToSent);
        if (which == ServerClient::kServer) {
            if (serverConn_->outputBuffer()->readableBytes() > 0) {
                clientConn_->stopRead();
                serverConn_->setWriteCompleteCallback(
                    std::bind(&Tunnel::onWriteCompleteWeak,
                              std::weak_ptr<Tunnel>(shared_from_this()),
                              ServerClient::kServer, _1));
            }
        } else {
            if (clientConn_->outputBuffer()->readableBytes() > 0) {
                serverConn_->stopRead();
                clientConn_->setWriteCompleteCallback(
                    std::bind(&Tunnel::onWriteCompleteWeak,
                              std::weak_ptr<Tunnel>(shared_from_this()),
                              ServerClient::kClient, _1));
            }
        }
    }

    static void onHighWaterMarkWeak(const std::weak_ptr<Tunnel>& wkTunnel,
                                    ServerClient which,
                                    const chaonet::TcpConnectionPtr& conn,
                                    size_t bytesToSent) {
        std::shared_ptr<Tunnel> tunnel = wkTunnel.lock();
        if (tunnel) {
            tunnel->onHighWaterMark(which, conn, bytesToSent);
        }
    }

    void onWriteComplete(ServerClient which,
                         const chaonet::TcpConnectionPtr& conn) {
        SPDLOG_INFO("{} onWriteComplete {}", (which == ServerClient::kServer ? "server" : "client"), conn->name());
        if (which == ServerClient::kServer) {
            clientConn_->startRead();
            serverConn_->setWriteCompleteCallback(chaonet::WriteCompleteCallback());
        } else {
            serverConn_->startRead();
            clientConn_->setWriteCompleteCallback(chaonet::WriteCompleteCallback());
        }
    }

    static void onWriteCompleteWeak(const std::weak_ptr<Tunnel>& wkTunnel,
                                    ServerClient which,
                                    const chaonet::TcpConnectionPtr& conn) {
        std::shared_ptr<Tunnel> tunnel = wkTunnel.lock();
        if (tunnel) {
            tunnel->onWriteComplete(which, conn);
        }
    }

   private:
    chaonet::TcpClient client_;
    chaonet::TcpConnectionPtr serverConn_;
    chaonet::TcpConnectionPtr clientConn_;
};

typedef std::shared_ptr<Tunnel> TunnelPtr;

#endif  // CHAONET_TUNNEL_H
