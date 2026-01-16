#include "../include/errors.h"
#include "../include/flags.h"
#include "../include/network.h"
#include "../include/renderer.h"
#include "../include/thread_queue.h"
#include "../include/files.h"
#include "../include/sprocess.h"

#include <functional>
#include <iostream>
#include <optional>
#include <string>

int init(const int argc, char** argv, MainThreadQueue* mainThreadQueue, ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager, ProcessManager* processManager)
{
    const Flags* flags = Flags::parse(argc, argv, errorHandler);

    if (!flags)
    {
        mainThreadQueue->execute(true);

        return 1;
    }

    if (flags->type == LaunchType::Service)
    {
        networkManager->beginService([=](const std::string ip)
        {
            std::vector<std::string> args = { "--receive", ip };

            if (!processManager->createProcess(args))
            {
                errorHandler->handle(SquirrelException("Failed to create process."));
            }
        });

        while (true)
        {
            mainThreadQueue->execute(true);
        }
    }

    else if (flags->type == LaunchType::Receive)
    {
        Renderer* renderer = new Renderer(mainThreadQueue, errorHandler, networkManager, fileManager);

        networkManager->beginReceive(flags->ip, [=](const std::string name, const std::string& data)
        {
            mainThreadQueue->push([=]()
            {
                renderer->setupReceive(name, data);
            });
        });

        renderer->setupMain();
        renderer->run();
    }

    else
    {
        Renderer* renderer = new Renderer(mainThreadQueue, errorHandler, networkManager, fileManager);

        renderer->setPath(flags->path);
        renderer->setupMain();
        renderer->run();
    }

    return 0;
}

#ifdef _WIN32

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    AllocConsole(); // remove after adding error dialogs

    freopen("CONOUT$", "w", stdout); // this too

    MainThreadQueue* mainThreadQueue = new MainThreadQueue();

    ErrorHandler* errorHandler = new ErrorHandler(mainThreadQueue);
    NetworkManager* networkManager = new WinNetworkManager(errorHandler);
    FileManager* fileManager = new WinFileManager();
    ProcessManager* processManager = new WinProcessManager(errorHandler);

    if (wcsnlen(pCmdLine, 1) > 0)
    {
        int argc = 0;

        LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc);

        char** argvChar = (char**)malloc(sizeof(char*) * argc);

        for (unsigned int i = 0; i < argc; i++)
        {
            unsigned int length = wcstombs(nullptr, argv[i], 0);

            argvChar[i] = (char*)malloc(sizeof(char) * length);

            wcstombs(argvChar[i], argv[i], length);

            argvChar[i][length] = '\0';
        }

        return init(argc, argvChar, mainThreadQueue, errorHandler, networkManager, fileManager, processManager);
    }

    return init(0, nullptr, mainThreadQueue, errorHandler, networkManager, fileManager, processManager);
}

#elif __APPLE__

int main(int argc, char** argv)
{
    MainThreadQueue* mainThreadQueue = new MainThreadQueue();

    ErrorHandler* errorHandler = new ErrorHandler(mainThreadQueue);
    NetworkManager* networkManager = new BSDNetworkManager(errorHandler);
    FileManager* fileManager = new MacFileManager();
    ProcessManager* processManager = new MacProcessManager(errorHandler);

    return init(argc - 1, argv + 1, mainThreadQueue, errorHandler, networkManager, fileManager, processManager);
}

#else

int main(int argc, char** argv)
{
    MainThreadQueue* mainThreadQueue = new MainThreadQueue();

    ErrorHandler* errorHandler = new ErrorHandler(mainThreadQueue);
    NetworkManager* networkManager = new BSDNetworkManager(errorHandler);
    FileManager* fileManager = new LinuxFileManager();
    ProcessManager* processManager = new LinuxProcessManager(errorHandler);

    return init(argc - 1, argv + 1, mainThreadQueue, errorHandler, networkManager, fileManager, processManager);
}

#endif
