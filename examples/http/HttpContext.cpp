//
// Created by chao on 2022/5/13.
//

#include "HttpContext.h"

#include "Buffer.h"
#include "HttpContext.h"

using namespace chaonet;

bool HttpContext::processRequestLine(const char* begin, const char *end) {
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    if (space != end && request_.setMethod(start, space)) {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end) {
            const char* question = std::find(start, space, '?');
            if (question != space) {
                request_.setPath(start, question);
                request_.setQuery(question, space);
            } else {
                request_.setPath(start, space);
            }
            start = space + 1;
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            if (succeed) {
                if (*(end - 1) == '1') {
                    request_.setVersion(HttpRequest::Version::kHttp11);
                } else if (*(end - 1) == '0') {
                    request_.setVersion(HttpRequest::Version::kHttp10);
                } else {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime) {
    bool ok = true;
    bool hasMore = true;
    while (hasMore) {
        if (state_ == HttpRequestParseState::kExpectRequestLine) {
            const char* crlf = buf->findCRLF();
            if (crlf) {
                ok = processRequestLine(buf->peek(), crlf);
                if (ok) {
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);
                    state_ = HttpRequestParseState::kExpectHeaders;
                } else {
                    hasMore = false;
                }
            } else {
                hasMore = false;
            }
        } else if (state_ == HttpRequestParseState::kExpectHeaders) {
            const char* crlf = buf->findCRLF();
            if (crlf) {
                const char* colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf) {
                    request_.addHeader(buf->peek(), colon, crlf);
                } else {
                    state_ = HttpRequestParseState::kGotAll;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2);
            }
        } else if (state_ == HttpRequestParseState::kExpectBody) {
            // todo;
        }
    }
    return ok;
}
