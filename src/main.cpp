#include "../include/gui.h"
#include "../include/network.h"
#include "../include/files.h"

void init(const int argc, char** argv, NetworkManager* networkManager, FileManager* fileManager)
{
    GUI* gui = new GUI(networkManager, fileManager);

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

        init(argc, argvChar, new WinNetworkManager(), new WinFileManager());
    }

    else
    {
        init(0, nullptr, new WinNetworkManager(), new WinFileManager());
    }

    return 0;
}

#elif __APPLE__

int main(int argc, char** argv)
{
    init(argc - 1, argv + 1, new BSDNetworkManager(), new MacFileManager());

    return 0;
}

#else

int main(int argc, char** argv)
{
    init(argc - 1, argv + 1, new BSDNetworkManager(), new LinuxFileManager());

    return 0;
}

#endif
