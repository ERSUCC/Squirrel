#pragma once

#include <exception>

#include "safe_queue.hpp"

struct SquirrelException : public std::exception
{
    SquirrelException(const std::string message);

    const char* what() const noexcept override;

private:
    const std::string message;

};

struct SquirrelSocketException : public SquirrelException
{
    SquirrelSocketException(const std::string message);
};

struct SquirrelFileException : public SquirrelException
{
    SquirrelFileException(const std::string message);
};

struct ErrorHandler : public ThreadSafeQueue<SquirrelException> {};
