//
// Created by chao on 2022/3/15.
//

#include <stdio.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Poller.h"

chaonet::EventLoop* g_loop;
int g_flag = 0;

void run4() {
    printf("run4(): pid = %d, flag = %d\n", getpid(), g_flag);
    g_loop->quit();
}

void run3() {
    printf("run3(): pid = %d, flag = %d\n", getpid(), g_flag);
    g_loop->runAfter(3, run4);
    g_flag = 3;
}

void run2() {
    printf("run2(): pid = %d, flag = %d\n", getpid(), g_flag);
    g_loop->queueInLoop(run3);
}

void run1() {
    g_flag = 1;
    printf("run1(): pid = %d, flag = %d\n", getpid(), g_flag);
    g_loop->runInLoop(run2);
    g_flag = 2;
}

void test1() {
    printf("main(): pid = %d, flag = %d\n", getpid(), g_flag);
    chaonet::EventLoop loop;
    g_loop = &loop;
    loop.runAfter(2, run1);
    loop.loop();
    printf("main(): pid = %d, flag = %d\n", getpid(), g_flag);
}

int main() {
    test1();
    return 0;
}
