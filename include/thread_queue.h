#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

struct MainThreadQueue
{
    void push(std::function<void()> function);
    void execute(const bool block);

private:
    std::queue<std::function<void()>> functions;

    std::mutex lock;
    std::condition_variable signal;

};
