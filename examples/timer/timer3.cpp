//
// Created by ps on 22-5-30.
//

#include "EventLoop.h"
#include <iostream>

class Printer {
   public:
    explicit Printer(chaonet::EventLoop* loop) : loop_(loop), count_(0) {
        loop_->runAfter(1, std::bind(&Printer::print, this));
    }

    Printer() = delete;
    Printer(const Printer&) = delete;
    Printer& operator=(const Printer&) = delete;

    ~Printer() {
        std::cout << "Final count is " << count_ << "\n";
    }

    void print() {
        if (count_ < 5) {
            std::cout << count_ << "\n";
            ++count_;

            loop_->runAfter(1, std::bind(&Printer::print, this));
        } else {
            loop_->quit();
        }
    }

   private:
    chaonet::EventLoop* loop_;
    int count_;
};

int main() {
    chaonet::EventLoop loop;
    Printer printer(&loop);
    loop.loop();

    return 0;
}
