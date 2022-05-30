//
// Created by ps on 22-5-29.
//

#ifndef CHAONET_PROTOBUFCODECLITE_H
#define CHAONET_PROTOBUFCODECLITE_H

#include <memory>
#include <string>

#include "Callbacks.h"
#include "utils/Timestamp.h"

using std::string;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

namespace chaonet {

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class ProtobufCodecLite {
   public:
    const static int kHeaderLen = sizeof(int32_t);
    const static int kChecksumLen = sizeof(int32_t);
    const static int kMaxMessageLen = 64 * 1024 * 1024;

    enum class ErrorCode {
        kNoError = 0,
        kInvalidLengeth,
        kCheckSumError,
        kInvalidNameLen,
        kUnkownMessageType,
        kParseError,
    };

    typedef std::function<bool(const TcpConnectionPtr&, std::string, Timestamp)>
        RawMessageCallback;
    typedef std::function<void(const TcpConnectionPtr&, const MessagePtr&,
                               Timestamp)>
        ProtobufMessageCallback;
    typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp,
                               ErrorCode)>
        ErrorCallback;

    ProtobufCodecLite(const ::google::protobuf::Message* prototype,
                      std::string tagArg,
                      const ProtobufMessageCallback& messageCb,
                      const RawMessageCallback& rawCb = RawMessageCallback(),
                      const ErrorCallback& errorCb = defaultErrorCallback)
        : prototype_(prototype),
          tag_(tagArg),
          messageCallback_(messageCb),
          errorCallback_(errorCb),
          kMinMessageLen(tagArg.size() + kChecksumLen) {}

    virtual ~ProtobufCodecLite() = default;

    const string& tag() const { return tag_; }

    void send(const TcpConnectionPtr& conn,
              const ::google::protobuf::Message& message);

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf,
                   Timestamp receiveTime);

    virtual bool parseFromBuffer(string buf,
                                 google::protobuf::Message* message);
    virtual int serializeToBuffer(const ::google::protobuf::Message& message,
                                  Buffer* buf);

    static const string& errorCodeToString(ErrorCode errorCode);

    ErrorCode parse(const char* buf, int len,
                    ::google::protobuf::Message* message);
    void fillEmptyBuffer(chaonet::Buffer* buf,
                         const ::google::protobuf::Message& message);

    static int32_t checksum(const void* buf, int len);
    static bool validateChecksum(const char* buf, int len);
    static int32_t asInt32(const char* buf);
    static void defaultErrorCallback(const TcpConnectionPtr&, Buffer*,
                                     Timestamp, ErrorCode);

   private:
    const ::google::protobuf::Message* prototype_;
    std::string tag_;
    ProtobufMessageCallback messageCallback_;
    RawMessageCallback rawCb_;
    ErrorCallback errorCallback_;
    int kMinMessageLen;
};

template <typename MSG, const char* TAG, typename CODEC = ProtobufCodecLite>
class ProtobufCodecLiteT {
    static_assert(std::is_base_of<ProtobufCodecLite, CODEC>::value,
                  "CODEC should be derived from ProtobufCodecLite");

   public:
    typedef std::shared_ptr<MSG> ConcreteMessagePtr;
    typedef std::function<void(const TcpConnectionPtr&,
                               const ConcreteMessagePtr&, Timestamp)>
        ProtobufMessageCallback;
    typedef ProtobufCodecLite::RawMessageCallback RawMessageCallback;
    typedef ProtobufCodecLite::ErrorCallback ErrorCallback;

    explicit ProtobufCodecLiteT(
        const ProtobufMessageCallback& messageCb,
        const RawMessageCallback& rawCb = RawMessageCallback(),
        const ErrorCallback& errorCb = ProtobufCodecLite::defaultErrorCallback)
        : messageCallback_(messageCb),
          codec_(&MSG::default_instance(), TAG,
                 std::bind(&ProtobufCodecLiteT::onRpcMessage, this, _1, _2, _3),
                 rawCb, errorCb) {}

    const string& tag() const { return codec_.tag(); }

    void send(const TcpConnectionPtr& conn, const MSG& message) {
        codec_.send(conn, message);
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime) {
        codec_.onMessage(conn, buf, receiveTime);
    }

    void onRpcMessage(const TcpConnectionPtr& conn, const MessagePtr& message, Timestamp receiveTime) {
        messageCallback_(conn,  reinterpret_cast<MSG>(message), receiveTime);
    }

    void fillEmptyBuffer(chaonet::Buffer* buf, const MSG& message) {
        codec_.fillEmptyBuffer(buf, message);
    }

   private:
    ProtobufMessageCallback messageCallback_;
    CODEC codec_;
};

}  // namespace chaonet

#endif  // CHAONET_PROTOBUFCODECLITE_H
