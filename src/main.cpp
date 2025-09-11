#include "../include/network.h"
#include "../include/renderer.h"
#include "../include/safe_queue.hpp"
#include "../include/files.h"

#include <functional>
#include <iostream>
#include <optional>

int init(const int argc, char** argv, ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager)
{
    if (argc == 0)
    {
        std::cout << "No arguments provided.\n";

        return 1;
    }

    if (strncmp(argv[0], "--send", 6) == 0)
    {
        Renderer* renderer = new Renderer(errorHandler, networkManager, fileManager);

        if (argc > 1)
        {
            renderer->setPath(argv[1]);
        }

        renderer->setupSend();
        renderer->run();
    }

    else if (strncmp(argv[0], "--receive", 9) == 0)
    {
        ThreadSafeQueue<std::function<void()>>* mainThreadQueue = new ThreadSafeQueue<std::function<void()>>();

        networkManager->beginListen([=](const std::string name, const std::string& data)
        {
            mainThreadQueue->push([=]()
            {
                Renderer* renderer = new Renderer(errorHandler, networkManager, fileManager);

                renderer->queueFunction([=]()
                {
                    renderer->setupReceive(name, data);
                });

                renderer->run();

                delete renderer;
            });
        });

        while (true)
        {
            if (std::optional<std::function<void()>> operation = mainThreadQueue->pop())
            {
                operation.value()();
            }
        }
    }

    else
    {
        std::cout << "Unknown argument \"" << argv[0] << "\".\n";

        return 1;
    }

    return 0;
}

#ifdef _WIN32

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    AllocConsole(); // remove after adding error dialogs

    freopen("CONOUT$", "w", stdout); // this too

    ErrorHandler* errorHandler = new ErrorHandler();

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

        return init(argc, argvChar, errorHandler, new WinNetworkManager(errorHandler), new WinFileManager());
    }

    return init(0, nullptr, errorHandler, new WinNetworkManager(errorHandler), new WinFileManager());
}

#elif __APPLE__

int main(int argc, char** argv)
{
    ErrorHandler* errorHandler = new ErrorHandler();

    return init(argc - 1, argv + 1, errorHandler, new BSDNetworkManager(errorHandler), new MacFileManager());
}

#else

int main(int argc, char** argv)
{
    ErrorHandler* errorHandler = new ErrorHandler();

    return init(argc - 1, argv + 1, errorHandler, new BSDNetworkManager(errorHandler), new LinuxFileManager());
}

#endif
