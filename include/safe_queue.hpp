#pragma once

#include <mutex>
#include <optional>
#include <queue>

template <typename T> struct ThreadSafeQueue
{
    void push(T value)
    {
        lock.lock();

        values.push(value);

        lock.unlock();
    }

    std::optional<T> pop()
    {
        lock.lock();

        if (values.empty())
        {
            lock.unlock();

            return std::nullopt;
        }

        T value = values.front();

        values.pop();

        lock.unlock();

        return value;
    }

private:
    std::mutex lock;

    std::queue<T> values;

};
