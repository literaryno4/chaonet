//
// Created by chao on 2022/5/13.
//

#include "HttpResponse.h"

#include <stdio.h>

#include "Buffer.h"

using namespace chaonet;

void HttpResponse::appendToBuffer(Buffer* output) const {
    char buf[32];
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", static_cast<int>(statusCode_));
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    if (closeConnection_) {
        output->append("Connection: close\r\n");
    } else {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    for (const auto& header : headers_) {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    output->append("\r\n");
    output->append(body_);
}
