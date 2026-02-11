#include "../include/thread_queue.h"

MainThreadQueue::MainThreadQueue() :
    mainThread(std::this_thread::get_id()) {}

void MainThreadQueue::push(const Action action)
{
    if (std::this_thread::get_id() == mainThread)
    {
        actions.push(action);
    }

    else
    {
        lock.lock();

        actions.push(action);

        lock.unlock();
    }

    signal.notify_all();
}

void MainThreadQueue::execute(const bool block)
{
    std::unique_lock<std::mutex> uniqueLock(lock);

    if (block)
    {
        while (actions.empty())
        {
            signal.wait(uniqueLock);
        }
    }

    if (!actions.empty())
    {
        actions.front()();
        actions.pop();
    }
}
