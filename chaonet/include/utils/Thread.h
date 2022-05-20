//
// Created by chao on 2022/3/30.
//

// in this file, we use RAII object to manage std::thread.

#ifndef CHAONET_THREAD_H
#define CHAONET_THREAD_H

#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <syscall.h>
#include <unistd.h>

namespace chaonet {

class Thread {
   public:
    typedef std::function<void()> ThreadFunc;
    explicit Thread(ThreadFunc threadFunc, std::string name = std::string())
        : threadFunc_(std::move(threadFunc)), name_(std::move(name)), started_(false) {}
    ~Thread() {
        thread_.detach();
    }
    void start() {
        if (!started()) {
            started_ = true;
            thread_ = std::thread(threadFunc_);
        }
    }
    void join() {
        if (started_ && thread_.joinable()) {
            thread_.join();
        }
    }
    bool started () const {
        return started_;
    }
    const std::string& getName() const {
        return name_;
    }

   private:
    ThreadFunc threadFunc_;
    std::thread thread_;
    std::string name_;
    bool started_;
};

inline pid_t getTid() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

}  // namespace chaonet
#endif  // CHAONET_THREAD_H
