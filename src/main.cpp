#include "../include/gui.h"
#include "../include/network.h"
#include "../include/files.h"

void init(const int argc, char** argv, ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager)
{
    GUI* gui = new GUI(errorHandler, networkManager, fileManager);

    if (argc == 0)
    {
        gui->setupEmpty();
    }

    else
    {
        gui->setupSend(argv[0]);
    }

    gui->run();
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

        init(argc, argvChar, errorHandler, new WinNetworkManager(errorHandler), new WinFileManager());
    }

    else
    {
        init(0, nullptr, errorHandler, new WinNetworkManager(errorHandler), new WinFileManager());
    }

    return 0;
}

#elif __APPLE__

int main(int argc, char** argv)
{
    ErrorHandler* errorHandler = new ErrorHandler();

    init(argc - 1, argv + 1, errorHandler, new BSDNetworkManager(errorHandler), new MacFileManager());

    return 0;
}

#else

int main(int argc, char** argv)
{
    ErrorHandler* errorHandler = new ErrorHandler();

    init(argc - 1, argv + 1, errorHandler, new BSDNetworkManager(errorHandler), new LinuxFileManager());

    return 0;
}

#endif
