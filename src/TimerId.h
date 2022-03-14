//
// Created by chao on 2022/3/14.
//

#ifndef CHAONET_TIMERID_H
#define CHAONET_TIMERID_H

namespace chaonet {

class Timer;

class TimerId {
   public:
    explicit TimerId(Timer* timer) : value_(timer) {}

   private:
    Timer* value_;
};

}  // namespace chaonet

#endif  // CHAONET_TIMERID_H
