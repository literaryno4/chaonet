//
// Created by ps on 22-5-30.
//

#include "EventLoop.h"
#include <iostream>

void print(chaonet::EventLoop* loop, int* count) {
    if (*count < 5) {
        std::cout << *count << "\n";
        ++(*count);
        loop->runAfter(1, std::bind(print, loop, count));
    } else {
        loop->quit();
    }
}

int main() {
    chaonet::EventLoop loop;
    int count = 0;
    loop.runAfter(1, std::bind(print, &loop, &count));
    loop.loop();
    std::cout << "Final count is " << count << "\n";
}
