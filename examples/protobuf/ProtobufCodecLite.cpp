//
// Created by ps on 22-5-29.
//

#include "ProtobufCodecLite.h"

#include <google/protobuf/message.h>
#include <spdlog/spdlog.h>
#include <zlib.h>

#include "google-inl.h"
#include "Buffer.h"
#include "SocketsOps.h"
#include "TcpConnection.h"
#include "assert.h"

using namespace chaonet;
using namespace chaonet::sockets;

namespace {
int ProtobufVersionCheck() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    return 0;
}
int __attribute__((unused)) dummy = ProtobufVersionCheck();
}  // namespace

void ProtobufCodecLite::send(const TcpConnectionPtr &conn,
                             const ::google::protobuf::Message &message) {
    Buffer buf;
    fillEmptyBuffer(&buf, message);
    conn->send(&buf);
}

void ProtobufCodecLite::fillEmptyBuffer(
    chaonet::Buffer *buf, const ::google::protobuf::Message &message) {
    assert(buf->readableBytes() == 0);

    buf->append(tag_);

    int byte_size = serializeToBuffer(message, buf);
    int32_t checkSum =
        checksum(buf->peek(), static_cast<int>(buf->readableBytes()));
    buf->appendInt32(checkSum);
    assert(buf->readableBytes() == tag_.size() + byte_size + kChecksumLen);
    (void)byte_size;
    int32_t len =
        sockets::hostToNetwork32(static_cast<int32_t>(buf->readableBytes()));
    buf->prepend(&len, sizeof(len));
}

void ProtobufCodecLite::onMessage(const TcpConnectionPtr &conn, Buffer *buf,
                                  Timestamp receiveTime) {
    while (buf->readableBytes() >=
           static_cast<uint32_t>(kMaxMessageLen + kHeaderLen)) {
        const int32_t len = buf->peekInt32();
        if (len > kMaxMessageLen || len < kMinMessageLen) {
            errorCallback_(conn, buf, receiveTime, ErrorCode::kInvalidLengeth);
            break;
        } else if (buf->readableBytes() >=
                   static_cast<size_t>(kHeaderLen + len)) {
            if (rawCb_ && !rawCb_(conn, string(buf->peek(), kHeaderLen + len),
                                  receiveTime)) {
                buf->retrieve(kHeaderLen + len);
                continue;
            }
            MessagePtr message(prototype_->New());
            ErrorCode errorCode =
                parse(buf->peek() + kHeaderLen, len, message.get());
            if (errorCode == ErrorCode::kNoError) {
                messageCallback_(conn, message, receiveTime);
                buf->retrieve(kHeaderLen + len);
            } else {
                errorCallback_(conn, buf, receiveTime, errorCode);
                break;
            }
        } else {
            break;
        }
    }
}

bool ProtobufCodecLite::parseFromBuffer(string buf,
                                        google::protobuf::Message *message) {
    return message->ParseFromArray(buf.data(), buf.size());
}

int ProtobufCodecLite::serializeToBuffer(
    const ::google::protobuf::Message &message, Buffer *buf) {
    GOOGLE_CHECK(message.IsInitialized())
        << InitializationErrorMessage("serialize", message);

#if GOOGLE_PROTOBUF_VERSION > 3009002
    int byte_size =
        google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
    int byte_size = message.ByteSize();
#endif
    buf->ensureWritableBytes(byte_size + kChecksumLen);
    uint8_t *start = reinterpret_cast<uint8_t *>(buf->beginWrite());
    uint8_t *end = message.SerializeWithCachedSizesToArray(start);
    if (end - start != byte_size) {
#if GOOGLE_PROTOBUF_VERSION > 3009002
        ByteSizeConsistencyError(
            byte_size,
            google::protobuf::internal::ToIntSize(message.ByteSizeLong()),
            static_cast<int>(end - start));
#else
        ByteSizeConsistencyError(byte_size, message.ByteSize(),
                                 static_cast<int>(end - start));
#endif
    }
    buf->hasWritten(byte_size);
    return byte_size;
}

namespace {
const string kNoErrorStr = "NoError";
const string kInvalidLengthStr = "InvalidLength";
const string kCheckSumErrorStr = "CheckSumError";
const string kInvalidNameLenStr = "InvalidNameLen";
const string kUnknownMessageTypeStr = "UnknownMessageType";
const string kParseErrorStr = "ParseError";
const string kUnknownErrorStr = "UnknownError";
}  // namespace

const string &ProtobufCodecLite::errorCodeToString(ErrorCode errorCode) {
    switch (errorCode) {
        case ErrorCode::kNoError:
            return kNoErrorStr;
        case ErrorCode::kInvalidLengeth:
            return kInvalidLengthStr;
        case ErrorCode::kCheckSumError:
            return kCheckSumErrorStr;
        case ErrorCode::kInvalidNameLen:
            return kInvalidNameLenStr;
        case ErrorCode::kUnkownMessageType:
            return kUnknownMessageTypeStr;
        case ErrorCode::kParseError:
            return kParseErrorStr;
        default:
            return kUnknownErrorStr;
    }
}

void ProtobufCodecLite::defaultErrorCallback(const TcpConnectionPtr &conn,
                                             Buffer *, Timestamp,
                                             ErrorCode errorCode) {
    SPDLOG_ERROR("ProtobufCodecLite::defaultErrorCallback - {}",
                 errorCodeToString(errorCode));
    if (conn && conn->connected()) {
        conn->shutdown();
    }
}

int32_t ProtobufCodecLite::asInt32(const char *buf) {
    int32_t be32 = 0;
    ::memcpy(&be32, buf, sizeof(be32));
    return sockets::networkToHost32(be32);
}

int32_t ProtobufCodecLite::checksum(const void *buf, int len) {
    return static_cast<int32_t>(::adler32(1, static_cast<const Bytef*>(buf), len));
}

bool ProtobufCodecLite::validateChecksum(const char *buf, int len) {
    int32_t expectedCheckSum = asInt32(buf + len - kChecksumLen);
    int32_t checkSum = checksum(buf, len - kChecksumLen);
    return checkSum == expectedCheckSum;
}

ProtobufCodecLite::ErrorCode ProtobufCodecLite::parse(
    const char *buf, int len, ::google::protobuf::Message *message) {
    ErrorCode error = ErrorCode::kNoError;
    if (validateChecksum(buf, len)) {
        if (memcmp(buf, tag_.data(), tag_.size()) == 0) {
            const char* data = buf + tag_.size();
            int32_t dataLen = len - kChecksumLen - static_cast<int>(tag_.size());
            if (parseFromBuffer(string(data, dataLen), message)) {
                error = ErrorCode::kNoError;
            } else {
                error = ErrorCode::kParseError;
            }
        } else {
            error = ErrorCode::kUnkownMessageType;
        }
    }
}
