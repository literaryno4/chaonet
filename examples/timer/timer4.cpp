//
// Created by chao on 22-5-30.
//

#include "EventLoop.h"
#include "EventLoopThread.h"
#include <iostream>

class Printer {
   public:
    Printer(chaonet::EventLoop* loop1, chaonet::EventLoop* loop2) : loop1_(loop1), loop2_(loop2) {
        loop1_->runAfter(1, std::bind(&Printer::print1, this));
        loop2_->runAfter(1, std::bind(&Printer::print2, this));
    }

    ~Printer() {
        std::cout << "Final count is " << count_ << "\n";
    }

    void print1() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ < 10) {
            std::cout << "Timer 1: " << count_ << "\n";
            ++count_;
            loop1_->runAfter(1, std::bind(&Printer::print1, this));
        } else {
            loop1_->quit();
        }
    }
    void print2() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ < 10) {
            std::cout << "Timer 2: " << count_ << "\n";
            ++count_;
            loop2_->runAfter(1, std::bind(&Printer::print2, this));
        } else {
            loop2_->quit();
        }
    }
   private:
    std::mutex mutex_;
    chaonet::EventLoop* loop1_;
    chaonet::EventLoop* loop2_;
    int count_;
};

int main() {
    std::unique_ptr<Printer> printer;

    chaonet::EventLoop loop;
    chaonet::EventLoopThread loopThread;
    chaonet::EventLoop* loopInAnotherThread = loopThread.startLoop();

    printer.reset(new Printer(&loop, loopInAnotherThread));
    loop.loop();

}