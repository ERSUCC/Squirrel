#include "../include/errors.h"

SquirrelException::SquirrelException(const std::string message) :
    message(message) {}

const char* SquirrelException::what() const noexcept
{
    return message.c_str();
}

SquirrelArgumentException::SquirrelArgumentException(const std::string message) :
    SquirrelException(message) {}

SquirrelSocketException::SquirrelSocketException(const std::string message) :
    SquirrelException(message) {}

SquirrelFileException::SquirrelFileException(const std::string message) :
    SquirrelException(message) {}

ErrorHandler::ErrorHandler(MainThreadQueue* mainThreadQueue) :
    mainThreadQueue(mainThreadQueue) {}

void ErrorHandler::handle(const SquirrelException& exception)
{
    mainThreadQueue->push([=]()
    {
        std::cout << exception.what() << "\n";
    });
}
