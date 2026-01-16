#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <string>

#include "thread_queue.h"

struct SquirrelException : public std::exception
{
    SquirrelException(const std::string message);

    const char* what() const noexcept override;

private:
    const std::string message;

};

struct SquirrelArgumentException : public SquirrelException
{
    SquirrelArgumentException(const std::string message);
};

struct SquirrelSocketException : public SquirrelException
{
    SquirrelSocketException(const std::string message);
};

struct SquirrelFileException : public SquirrelException
{
    SquirrelFileException(const std::string message);
};

struct ErrorHandler
{
    ErrorHandler(MainThreadQueue* mainThreadQueue);

    void handle(const SquirrelException& exception);

private:
    MainThreadQueue* mainThreadQueue;

};
