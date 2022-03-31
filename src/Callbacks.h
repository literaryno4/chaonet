//
// Created by chao on 2022/3/14.
//

#ifndef CHAONET_CALLBACKS_H
#define CHAONET_CALLBACKS_H

#include <functional>
#include <memory>

#include "utils/Timestamp.h"

namespace chaonet {

class TcpConnection;
class Buffer;

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef std::function<void()> TimerCallback;
typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr&, Buffer* buf,
                           Timestamp)>
    MessageCallback;


}  // namespace chaonet

#endif  // CHAONET_CALLBACKS_H
