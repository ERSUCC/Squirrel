#include "../include/errors.h"

SquirrelException::SquirrelException(const std::string message) :
    message(message) {}

const char* SquirrelException::what() const noexcept
{
    return message.c_str();
}

SquirrelSocketException::SquirrelSocketException(const std::string message) :
    SquirrelException(message) {}

SquirrelFileException::SquirrelFileException(const std::string message) :
    SquirrelException(message) {}
