//
// Created by chao on 2022/5/13.
//

#ifndef CHAONET_HTTPRESPONSE_H
#define CHAONET_HTTPRESPONSE_H

#include <map>

using std::string;

namespace chaonet {
class Buffer;
class HttpResponse {
   public:
    enum class HttpStatusCode {
        kUnknown,
        k200k = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

    explicit HttpResponse(bool close)
        : statusCode_(HttpStatusCode::kUnknown), closeConnection_(close) {}

    void setStatusCode(HttpStatusCode code) {
        statusCode_ = code;
    }

    void setStatusMessage(const string& message) {
        statusMessage_ = message;
    }

    void setCloseConnection(bool on) {
        closeConnection_ = on;
    }

    bool closeConnection() const {
        return closeConnection_;
    }

    void setContentType(const string& contentType) {
        addHeader("Content-Type", contentType);
    }

    void addHeader(const string& key, const string& value) {
        headers_[key] = value;
    }

    void setBody(const string& body) {
        body_ = body;
    }

    void appendToBuffer(Buffer* output) const;

   private:
    std::map<string, string> headers_;
    HttpStatusCode statusCode_;
    string statusMessage_;
    bool closeConnection_;
    string body_;
};
}  // namespace chaonet
#endif  // CHAONET_HTTPRESPONSE_H
