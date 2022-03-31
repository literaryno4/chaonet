//
// Created by chao on 2022/3/31.
//

#ifndef CHAONET_TOOLS_H
#define CHAONET_TOOLS_H

#include <spdlog/spdlog.h>

class EventLoop;

template<typename T>
T* checkNotNUll(T* ptr) {
    if (ptr == nullptr) {
        SPDLOG_ERROR("ptr is null!");
    }
    return ptr;
}

#endif  // CHAONET_TOOLS_H
