//
// Created by chao on 2022/3/15.
//

#ifndef CHAONET_SOCKET_H
#define CHAONET_SOCKET_H



namespace chaonet {

class InetAddress;

class Socket {
   public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    ~Socket();

    int fd() const {return sockfd_;}

    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);
    void setReuseAddr(bool on);
    void shutdownWrite();
    void setTcpNoDelay(bool on);

   private:
    const int sockfd_;
};

}

#endif  // CHAONET_SOCKET_H
