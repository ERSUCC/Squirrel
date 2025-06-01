#pragma once

#include <filesystem>
#include <string>

struct FileManager
{
    virtual std::filesystem::path getSavePath(const std::string name) const = 0;
};

#ifdef _WIN32

#include <combaseapi.h>
#include <ShObjIdl.h>

struct WinFileManager : public FileManager
{
    std::filesystem::path getSavePath(const std::string name) const override;
};

#elif __APPLE__

struct MacFileManager : public FileManager
{
    std::filesystem::path getSavePath(const std::string name) const override;
};

#else

#include <limits.h>
#include <stdio.h>

struct LinuxFileManager : public FileManager
{
    std::filesystem::path getSavePath(const std::string name) const override;
};

#endif
