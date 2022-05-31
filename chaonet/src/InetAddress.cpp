//
// Created by chao on 2022/3/15.
//

#include "InetAddress.h"

#include <spdlog/spdlog.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>

#include "SocketsOps.h"

using namespace chaonet;

static const in_addr_t kInaddrAny = INADDR_ANY;
static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in), "");

InetAddress::InetAddress(uint64_t port) {
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(kInaddrAny);
    addr_.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(const std::string &ip, uint16_t port) {
    bzero(&addr_, sizeof(addr_));
    sockets::fromHostPort(ip.c_str(), port, &addr_);
}

std::string InetAddress::toHostPort() const {
    char buf[32];
    sockets::toHostPort(buf, sizeof(buf), addr_);
    return buf;
}

static __thread char t_resolveBuffer[64* 1024];

bool InetAddress::resolve(std::string hostname, InetAddress *out) {
    assert(out != nullptr);
    struct hostent hent;
    struct hostent* he = nullptr;
    int herrno = 0;
    memset(&hent, 0, sizeof(hent));

    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof(t_resolveBuffer), &he, &herrno);
    if (ret == 0 && he != nullptr) {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        return true;
    } else {
        if (ret) {
            SPDLOG_ERROR("InetAddress::resolve");
        }
        return false;
    }
}
