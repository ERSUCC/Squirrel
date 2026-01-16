#include "../include/sprocess.h"

ProcessManager::ProcessManager(ErrorHandler* errorHandler) :
    errorHandler(errorHandler) {}

#ifdef _WIN32

WinProcessManager::WinProcessManager(ErrorHandler* errorHandler) :
    ProcessManager(errorHandler) {}

bool WinProcessManager::createProcess(std::vector<std::string>& args) const
{
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;

    memset(&startupInfo, 0, sizeof(STARTUPINFO));
    memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));

    char path[MAX_PATH + 1];

    const DWORD pathLength = GetModuleFileName(nullptr, path, MAX_PATH);

    size_t commandLength = pathLength + 2;

    for (const std::string& arg : args)
    {
        commandLength += arg.size() + 1;
    }

    char* fullCommand = (char*)malloc(sizeof(char) * (commandLength + 1));

    fullCommand[0] = '"';

    strncpy(fullCommand + 1, path, pathLength);

    fullCommand[pathLength + 1] = '"';

    size_t pos = pathLength + 2;

    for (const std::string& arg : args)
    {
        fullCommand[pos++] = ' ';

        strncpy(fullCommand + pos, arg.c_str(), arg.size());

        pos += arg.size();
    }

    fullCommand[commandLength] = '\0';

    const bool result = CreateProcess(path, fullCommand, nullptr, nullptr, false, 0, nullptr, nullptr, &startupInfo, &processInfo);

    free(fullCommand);

    return result != 0;
}

#elif __APPLE__

MacProcessManager::MacProcessManager(ErrorHandler* errorHandler) :
    ProcessManager(errorHandler) {}

bool MacProcessManager::createProcess(std::vector<std::string>& args) const
{
    const pid_t child = fork();

    if (child == -1)
    {
        return false;
    }

    if (child)
    {
        return true;
    }

    if (setsid() == -1)
    {
        return false;
    }

    char** command = (char**)malloc(sizeof(char*) * (args.size() + 2));

    std::string exec = getExecutablePath();

    command[0] = exec.data();

    for (size_t i = 0; i < args.size(); i++)
    {
        command[i + 1] = args[i].data();
    }

    command[args.size() + 1] = nullptr;

    return execv(command[0], command) != -1;
}

#else

LinuxProcessManager::LinuxProcessManager(ErrorHandler* errorHandler) :
    ProcessManager(errorHandler) {}

bool LinuxProcessManager::createProcess(std::vector<std::string>& args) const
{
    const pid_t child = fork();

    if (child == -1)
    {
        return false;
    }

    if (child)
    {
        return true;
    }

    if (setsid() == -1)
    {
        return false;
    }

    char** command = (char**)malloc(sizeof(char*) * (args.size() + 2));

    command[0] = (char*)malloc(sizeof(char) * (PATH_MAX + 1));

    const ssize_t execLength = readlink("/proc/self/exe", command[0], sizeof(char) * PATH_MAX);

    if (execLength == -1)
    {
        return false;
    }

    command[0][execLength] = '\0';

    for (size_t i = 0; i < args.size(); i++)
    {
        command[i + 1] = args[i].data();
    }

    command[args.size() + 1] = nullptr;

    return execv(command[0], command) != -1;
}

#endif
