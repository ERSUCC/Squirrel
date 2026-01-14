#include "../include/network.h"
#include "../include/renderer.h"
#include "../include/safe_queue.hpp"
#include "../include/files.h"

#include <functional>
#include <iostream>
#include <optional>

int init(const int argc, char** argv, ThreadSafeQueue<std::function<void()>>* mainThreadQueue, ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager)
{
    if (argc > 0 && strncmp(argv[0], "--service", 9) == 0)
    {
        networkManager->beginService([=](const std::string name, const std::string& data)
        {
            mainThreadQueue->push([=]()
            {
                Renderer* renderer = new Renderer(mainThreadQueue, errorHandler, networkManager, fileManager);

                renderer->setupReceive(name, data);

                delete renderer;
            });
        });

        while (true)
        {
            while (std::optional<SquirrelException> error = errorHandler->pop())
            {
                std::cout << error.value().what() << "\n";
            }

            if (const std::optional<std::function<void()>> function = mainThreadQueue->pop())
            {
                function.value()();
            }
        }
    }

    else
    {
        Renderer* renderer = new Renderer(mainThreadQueue, errorHandler, networkManager, fileManager);

        if (argc != 0)
        {
            if (!std::filesystem::exists(argv[0]))
            {
                std::cout << "Specified file does not exist.\n";

                return 1;
            }

            renderer->setPath(argv[0]);
        }

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

    ThreadSafeQueue<std::function<void()>>* mainThreadQueue = new ThreadSafeQueue<std::function<void()>>();

    ErrorHandler* errorHandler = new ErrorHandler();
    NetworkManager* networkManager = new WinNetworkManager(errorHandler);

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

        return init(argc, argvChar, mainThreadQueue, errorHandler, networkManager, new WinFileManager());
    }

    return init(0, nullptr, mainThreadQueue, errorHandler, networkManager, new WinFileManager());
}

#elif __APPLE__

int main(int argc, char** argv)
{
    ThreadSafeQueue<std::function<void()>>* mainThreadQueue = new ThreadSafeQueue<std::function<void()>>();

    ErrorHandler* errorHandler = new ErrorHandler();
    NetworkManager* networkManager = new BSDNetworkManager(errorHandler);

    return init(argc - 1, argv + 1, mainThreadQueue, errorHandler, networkManager, new MacFileManager());
}

#else

int main(int argc, char** argv)
{
    ThreadSafeQueue<std::function<void()>>* mainThreadQueue = new ThreadSafeQueue<std::function<void()>>();

    ErrorHandler* errorHandler = new ErrorHandler();
    NetworkManager* networkManager = new BSDNetworkManager(errorHandler);

    return init(argc - 1, argv + 1, mainThreadQueue, errorHandler, networkManager, new LinuxFileManager());
}

#endif
