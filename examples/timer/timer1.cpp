//
// Created by ps on 22-5-30.
//

#include <iostream>

#include "EventLoop.h"

int main() {
    chaonet::EventLoop loop;
    loop.runAfter(5, []() { std::cout << "Hello World\n"; });
    loop.loop();
}