//
// Created by chao on 2022/3/14.
//

#ifndef CHAONET_TIMERID_H
#define CHAONET_TIMERID_H

#include <memory>

namespace chaonet {

class Timer;

class TimerId {
   public:
    explicit TimerId(std::shared_ptr<Timer> timer = nullptr, int64_t seq = 0)
        : timer_(timer), sequence_(seq) {}
    std::weak_ptr<Timer> timer() {
        return timer_;
    }
    int64_t sequence() {
        return sequence_;
    }

   private:
    std::weak_ptr<Timer> timer_;
    int64_t sequence_;
};

}  // namespace chaonet

#endif  // CHAONET_TIMERID_H
