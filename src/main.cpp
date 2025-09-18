#include "../include/network.h"
#include "../include/renderer.h"
#include "../include/safe_queue.hpp"
#include "../include/service.h"
#include "../include/files.h"

#include <functional>
#include <iostream>
#include <optional>

int init(const int argc, char** argv, ThreadSafeQueue<std::function<void()>>* mainThreadQueue, ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager, ServiceManager* serviceManager)
{
    if (argc == 0)
    {
        std::cout << "No arguments provided.\n";

        return 1;
    }

    if (strncmp(argv[0], "--app", 6) == 0)
    {
        Renderer* renderer = new Renderer(mainThreadQueue, errorHandler, networkManager, fileManager, serviceManager);

        if (argc > 1)
        {
            renderer->setPath(argv[1]);
        }

        renderer->setupMain();
        renderer->run();
    }

    else if (strncmp(argv[0], "--service", 9) == 0)
    {
        serviceManager->startService();

        networkManager->beginService([=](const std::string name, const std::string ip)
        {
            const size_t length = name.size() + 9;

            char* data = (char*)malloc(sizeof(char) * length);

            data[0] = MessageConstants::RESPONSE;

            *(uint16_t*)(data + 2) = length;
            *(uint32_t*)(data + 4) = networkManager->convertAddress(ip);

            strncpy(data + 8, name.c_str(), name.size());

            data[length - 1] = '\0';

            serviceManager->writeMessage(MessageType::Application, new DataArray(data, length));
        }, std::bind(&NetworkManager::beginReceive, networkManager, std::placeholders::_1, [=](const std::string name, const std::string& data)
        {
            mainThreadQueue->push([=]()
            {
                Renderer* renderer = new Renderer(mainThreadQueue, errorHandler, networkManager, fileManager, serviceManager);

                renderer->setupReceive(name, data);

                delete renderer;
            });
        }));

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

        return init(argc, argvChar, mainThreadQueue, errorHandler, networkManager, new WinFileManager(), new WindowsServiceManager(errorHandler, networkManager));
    }

    return init(0, nullptr, mainThreadQueue, errorHandler, networkManager, new WinFileManager(), new WindowsServiceManager(errorHandler, networkManager));
}

#elif __APPLE__

int main(int argc, char** argv)
{
    ThreadSafeQueue<std::function<void()>>* mainThreadQueue = new ThreadSafeQueue<std::function<void()>>();

    ErrorHandler* errorHandler = new ErrorHandler();
    NetworkManager* networkManager = new BSDNetworkManager(errorHandler);

    return init(argc - 1, argv + 1, mainThreadQueue, errorHandler, networkManager, new MacFileManager(), new POSIXServiceManager(errorHandler, networkManager));
}

#else

int main(int argc, char** argv)
{
    ThreadSafeQueue<std::function<void()>>* mainThreadQueue = new ThreadSafeQueue<std::function<void()>>();

    ErrorHandler* errorHandler = new ErrorHandler();
    NetworkManager* networkManager = new BSDNetworkManager(errorHandler);

    return init(argc - 1, argv + 1, mainThreadQueue, errorHandler, networkManager, new LinuxFileManager(), new POSIXServiceManager(errorHandler, networkManager));
}

#endif
