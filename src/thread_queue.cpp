#include "../include/thread_queue.h"

void MainThreadQueue::push(std::function<void()> function)
{
    lock.lock();

    functions.push(function);

    lock.unlock();

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
