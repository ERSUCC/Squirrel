#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

struct MainThreadQueue
{
    typedef std::function<void()> Action;

    MainThreadQueue();

    void push(const Action action);
    void execute(const bool block);

private:
    const std::thread::id mainThread;

    std::queue<Action> actions;

    std::mutex lock;
    std::condition_variable signal;

};
