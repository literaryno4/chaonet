//
// Created by chao on 22-5-30.
//

#include "EventLoop.h"
#include "EventLoopThread.h"
#include <iostream>

class Printer {
   public:
    Printer(chaonet::EventLoop* loop1, chaonet::EventLoop* loop2) : loop1_(loop1), loop2_(loop2), count_(0) {
        loop1_->runAfter(1, std::bind(&Printer::print1, this));
        loop2_->runAfter(1, std::bind(&Printer::print2, this));
    }

    ~Printer() {
        std::cout << "Final count is " << count_ << "\n";
    }

    void print1() {
        bool shouldQuit = false;
        int count = 0;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (count_ < 10) {
                count = count_;
                ++count_;
            } else {
                shouldQuit = true;
            }
        }

        if (shouldQuit) {
            loop1_->quit();
        } else {
//            std::cout << "Timer 1: " << count << "\n";
            printf("Timer 1: %d\n", count);
            loop1_->runAfter(1, std::bind(&Printer::print1, this));
        }
    }
    void print2() {
        bool shouldQuit = false;
        int count = 0;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (count_ < 10) {
                count = count_;
                ++count_;
            } else {
                shouldQuit = true;
            }
        }

        if (shouldQuit) {
            loop1_->quit();
        } else {
//            std::cout << "Timer 2: " << count << "\n";
            printf("Timer 2: %d\n", count);
            loop2_->runAfter(1, std::bind(&Printer::print2, this));
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