//
// Created by chao on 2022/5/13.
//

#ifndef CHAONET_HTTPCONTEXT_H
#define CHAONET_HTTPCONTEXT_H

#include "HttpRequest.h"

namespace chaonet {

class Buffer;

class HttpContext {
   public:
    enum class HttpRequestParseState {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    HttpContext() : state_(HttpRequestParseState::kExpectRequestLine) {}

    bool parseRequest(Buffer* buf, Timestamp receiveTime);

    bool gotAll() const {
        return state_ == HttpRequestParseState::kGotAll;
    }

    void reset() {
        state_ = HttpRequestParseState::kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const {
        return request_;
    }

    HttpRequest& request() {
        return request_;
    }

   private:
    bool processRequestLine(const char* begin, const char* end);

    HttpRequestParseState state_;
    HttpRequest request_;
};

}
#endif  // CHAONET_HTTPCONTEXT_H
