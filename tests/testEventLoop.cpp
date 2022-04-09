//
// Created by chao on 2022/3/13.
//
#include <stdio.h>
#include <sys/timerfd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <strings.h>
#include <spdlog/spdlog.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Poller.h"
#include "utils/Thread.h"

using namespace chaonet;

chaonet::EventLoop* g_loop;
chaonet::TimerId toCancel;
int cnt = 0;

void printTid() {
    printf("pid = %d, tid = %d\n", getpid(), static_cast<pid_t>(::syscall(SYS_gettid)));
    printf("now %s\n", Timestamp::now().toString().c_str());
}

void print(const char* msg) {
    printf("msg %s %s\n", Timestamp::now().toString().c_str(), msg);
    if (++cnt == 20) {
        g_loop->quit();
    }
}

void threadFunc1() {
    printf("threadFunc(): pid = %d, tid = %d\n", getpid(),
           static_cast<pid_t>(::syscall(SYS_gettid)));

    chaonet::EventLoop loop;
    loop.loop();
}

void threadFunc2() { g_loop->loop(); }

void timeout(Timestamp) {
    printf("Timeout!\n");
    g_loop->quit();
}

void cancelSelf() {
    printf("cancelSelf()\n");
    g_loop->cancel(toCancel);
}

void test1() {
    printf("main(): pid = %d, tid = %d\n", getpid(),
           static_cast<pid_t>(::syscall(SYS_gettid)));
    chaonet::EventLoop loop;
    Thread thread(threadFunc1);
    thread.start();
    loop.loop();
    pthread_exit(NULL);
}

void test2() {
    chaonet::EventLoop loop;
    g_loop = &loop;
    Thread t(threadFunc2);
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
    loop.runAfter(10, std::bind(print, "once1"));
    loop.runAfter(10, std::bind(print, "once1.5"));
    loop.runAfter(2.5, std::bind(print, "once2.5"));
    loop.runAfter(3.5, std::bind(print, "once3.5"));
    chaonet::TimerId t = loop.runEvery(3, std::bind(print, "every2"));
    loop.runEvery(3, std::bind(print, "every3"));
    loop.runAfter(10, std::bind(&chaonet::EventLoop::cancel, &loop, t));
    SPDLOG_INFO("timer {} will be erased", t.sequence());
//    loop.runEvery(5, cancelSelf);
    toCancel = t;

    loop.loop();
    print("main loop exits");
    sleep(1);
}

void print1() {
    printf("yahaha");
    g_loop->quit();
}

void threadFunc() { g_loop->runAfter(1.0, print1); }

void test6() {
    chaonet::EventLoop loop;
    g_loop = &loop;
    Thread t(threadFunc);
    t.start();
    loop.loop();
}

int main() {
    //    test1();
    //    test2();
    //    test3();
    //    test4();
        test5();
//    test6();

    return 0;
}
