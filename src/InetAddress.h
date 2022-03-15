//
// Created by chao on 2022/3/15.
//

#ifndef CHAONET_INETADDRESS_H
#define CHAONET_INETADDRESS_H

#include <netinet/in.h>
#include <string>

namespace chaonet {

class InetAddress {
   public:
    explicit InetAddress(uint64_t port);

    InetAddress(const std::string& ip, uint16_t port);

    InetAddress(const struct sockaddr_in& addr): addr_(addr) {}

    const struct sockaddr_in& getSockAddrInet() const { return addr_;}
    void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr;}

    std::string toHostPort() const;

   private:
    struct sockaddr_in addr_;
};

}  // namespace chaonet

#endif  // CHAONET_INETADDRESS_H
