#include "../include/gui.h"
#include "../include/network.h"

void init(const int argc, char** argv, Socket* socket)
{
    GUI* gui = new GUI(socket);

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

        init(argc, argvChar, new WinSocket());
    }

    else
    {
        init(0, nullptr, new WinSocket());
    }

    return 0;
}

#else

int main(int argc, char** argv)
{
    init(argc - 1, argv + 1, new BSDSocket());

    return 0;
}

#endif
