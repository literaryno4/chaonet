//
// Created by chao on 2022/3/13.
//
#include "../EventLoop.h"
#include "../Channel.h"
#include "../Poller.h"

#include "Thread.h"
#include "CurrentThread.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/timerfd.h>

chaonet::EventLoop* g_loop;
int cnt = 0;

void printTid() {
    printf("pid = %d, tid = %d\n", getpid(), muduo::CurrentThread::tid());
    printf("now %s\n", muduo::Timestamp::now().toString().c_str());
}

void print(const char* msg) {
    printf("msg %s %s\n", muduo::Timestamp::now().toString().c_str(), msg);
    if (++cnt == 20) {
        g_loop->quit();
    }
}

void threadFunc1() {
    printf("threadFunc(): pid = %d, tid = %d\n", getpid(), muduo::CurrentThread::tid());

    chaonet::EventLoop loop;
    loop.loop();
}

void threadFunc2() {
    g_loop->loop();
}

void timeout() {
    printf("Timeout!\n");
    g_loop->quit();
}

void test1() {
    printf("main(): pid = %d, tid = %d\n", getpid(), muduo::CurrentThread::tid());
    chaonet::EventLoop loop;
    muduo::Thread thread(threadFunc1);
    thread.start();
    loop.loop();
    pthread_exit(NULL);
}

void test2() {
    chaonet::EventLoop loop;
    g_loop = &loop;
    muduo::Thread t(threadFunc2);
    t.start();
    t.join();
}

void test3() {
    chaonet::EventLoop loop1;
    chaonet::EventLoop loop2;
    loop1.loop();
    loop2.loop();
}

void test4() {
    chaonet::EventLoop loop;
    g_loop = &loop;

    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    chaonet::Channel channel(&loop, timerfd);
    channel.setReadCallback(timeout);
    channel.enableReading();

    struct itimerspec howlong;
    bzero(&howlong, sizeof(howlong));
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timerfd, 0, &howlong, NULL);

    loop.loop();
    ::close(timerfd);
}

void test5() {
    printTid();
    chaonet::EventLoop loop;
    g_loop = &loop;

    print("main");
    loop.runAfter(1, std::bind(print, "once1"));
    loop.runAfter(1.5, std::bind(print, "once1.5"));
    loop.runAfter(2.5, std::bind(print, "once2.5"));
    loop.runAfter(3.5, std::bind(print, "once3.5"));
    loop.runEvery(2, std::bind(print, "every2"));
    loop.runEvery(3, std::bind(print, "every3"));

    loop.loop();
    print("main loop exits");
    sleep(1);
}

int main() {
//    test1();
//    test2();
//    test3();
//    test4();
    test5();

    return 0;
}


