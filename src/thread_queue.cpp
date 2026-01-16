#include "../include/thread_queue.h"

MainThreadQueue::MainThreadQueue() :
    mainThread(std::this_thread::get_id()) {}

void MainThreadQueue::push(std::function<void()> function)
{
    if (std::this_thread::get_id() == mainThread)
    {
        functions.push(function);
    }

    else
    {
        lock.lock();

        functions.push(function);

        lock.unlock();
    }

    signal.notify_all();
}

void MainThreadQueue::execute(const bool block)
{
    std::unique_lock<std::mutex> uniqueLock(lock);

    if (block)
    {
        while (functions.empty())
        {
            signal.wait(uniqueLock);
        }
    }

    if (!functions.empty())
    {
        functions.front()();
        functions.pop();
    }
}
