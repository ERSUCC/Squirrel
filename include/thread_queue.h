#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

struct MainThreadQueue
{
    MainThreadQueue();

    void push(std::function<void()> function);
    void execute(const bool block);

private:
    const std::thread::id mainThread;

    std::queue<std::function<void()>> functions;

    std::mutex lock;
    std::condition_variable signal;

};
