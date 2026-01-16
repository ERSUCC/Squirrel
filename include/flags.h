#pragma once

#include <filesystem>
#include <string>

#include "errors.h"

enum LaunchType
{
    Service,
    Receive,
    General
};

struct Flags
{
    static Flags* parse(const int argc, char** argv, ErrorHandler* errorHandler);

    LaunchType type = LaunchType::General;

    std::string path;
    std::string ip;
};
