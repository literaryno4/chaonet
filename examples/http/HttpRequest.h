//
// Created by chao on 2022/5/13.
//

#ifndef CHAONET_HTTPREQUEST_H
#define CHAONET_HTTPREQUEST_H

#include <assert.h>
#include <stdio.h>

#include <map>
#include <unordered_map>

#include "utils/Timestamp.h"

using std::string;

namespace chaonet {

const std::unordered_map<string, string> fileType = {
    {"txt", "text/plain"},      {"c", "text/plain"},
    {"h", "text/plain"},        {"html", "text/html"},
    {"htm", "text/htm"},        {"css", "text/css"},
    {"gif", "image/gif"},       {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},     {"png", "image/png"},
    {"pdf", "application/pdf"}, {"ps", "application/postscript"},
};

class HttpRequest {
   public:
    enum class Method { kInvalid, kGet, kPost, kHead, kPut, kDelete };
    enum class Version { kUnknown, kHttp10, kHttp11 };

    HttpRequest() : method_(Method::kInvalid), version_(Version::kUnknown) {}

    void setVersion(Version v) { version_ = v; }

    bool setMethod(const char* start, const char* end) {
        assert(method_ == Method::kInvalid);
        string m(start, end);
        if (m == "GET") {
            method_ = Method::kGet;
        } else if (m == "POST") {
            method_ = Method::kPost;
        } else if (m == "HEAD") {
            method_ = Method::kHead;
        } else if (m == "PUT") {
            method_ = Method::kPut;
        } else if (m == "DELETE") {
            method_ = Method::kDelete;
        } else {
            method_ = Method::kInvalid;
        }
        return method_ != Method::kInvalid;
    }

    Method method() const { return method_; }

    const char* methodString() const {
        const char* result = "UNKNOWN";
        if (method_ == Method::kGet) {
            result = "GET";
        } else if (method_ == Method::kPost) {
            result = "POST";
        } else if (method_ == Method::kHead) {
            result = "HEAD";
        } else if (method_ == Method::kPut) {
            result = "PUT";
        } else if (method_ == Method::kDelete) {
            result = "DELETE";
        }

        return result;
    }

    void setPath(const char* start, const char* end) {
        path_.assign(start, end);
    }
    const string& path() const { return path_; }

    void setQuery(const char* start, const char* end) {
        query_.assign(start, end);
    }
    const string& query() const { return query_; }

    Version getVersion() const { return version_; }

    void setReceiveTime(Timestamp t) { receiveTime_ = t; }
    Timestamp receiveTime() const { return receiveTime_; }

    void addHeader(const char* start, const char* colon, const char* end) {
        string field(start, colon);
        ++colon;
        while (colon < end && std::isspace(*colon)) {
            ++colon;
        }
        string value(colon, end);
        while (!value.empty() && std::isspace(value[value.size() - 1])) {
            value.resize(value.size() - 1);
        }
        headers_[field] = value;
    }

    string getHeader(const string& field) const {
        string result;
        auto it = headers_.find(field);
        if (it != headers_.end()) {
            result = it->second;
        }
        return result;
    }

    const std::map<string, string>& headers() const { return headers_; }

    void swap(HttpRequest& that) {
        std::swap(method_, that.method_);
        std::swap(version_, that.version_);
        path_.swap(that.path_);
        query_.swap(that.query_);
        receiveTime_.swap(that.receiveTime_);
        headers_.swap(that.headers_);
    }

    string getFileType() const {
        if (path_ == "/") {
            return fileType.at("html");
        }
        int i;
        for (i = static_cast<int>(path_.size()) - 1; i >= 0; --i) {
            if (path_[i] == '.') {
                break;
            }
        }
        string suffix = path_.substr(i + 1);
        if (fileType.count(suffix) == 0 || i == 0) {
            return fileType.at("txt");
        }
        return fileType.at(suffix);
    }

   private:
    Method method_;
    Version version_;
    string path_;
    string query_;
    Timestamp receiveTime_;
    std::map<string, string> headers_;
};
}  // namespace chaonet

#endif  // CHAONET_HTTPREQUEST_H
