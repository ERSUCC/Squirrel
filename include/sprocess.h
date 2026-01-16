#pragma once

#include <string>
#include <vector>

#include "errors.h"

struct ProcessManager
{
    ProcessManager(ErrorHandler* errorHandler);

    virtual bool createProcess(std::vector<std::string>& args) const = 0;

protected:
    ErrorHandler* errorHandler;

};

#ifdef _WIN32

#include <Windows.h>

struct WinProcessManager : public ProcessManager
{
    WinProcessManager(ErrorHandler* errorHandler);

    bool createProcess(std::vector<std::string>& args) const override;
};

#elif __APPLE__

#include <unistd.h>

struct MacProcessManager : public ProcessManager
{
    MacProcessManager(ErrorHandler* errorHandler);

    bool createProcess(std::vector<std::string>& args) const override;

private:
    std::string getExecutablePath() const;

};

#else

#include <linux/limits.h>
#include <unistd.h>

struct LinuxProcessManager : public ProcessManager
{
    LinuxProcessManager(ErrorHandler* errorHandler);

    bool createProcess(std::vector<std::string>& args) const override;
};

#endif
