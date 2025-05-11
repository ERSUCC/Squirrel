#include <fstream>
#include <sstream>

#include "../include/gui.h"

void init(const int argc, char** argv)
{
    GUI* gui = new GUI();

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
    int argc = 0;

    LPWSTR* argv = nullptr;

    if (wcsnlen(pCmdLine, 1) > 0)
    {
        argv = CommandLineToArgvW(pCmdLine, &argc);
    }

    init(argc, argv);

    GUI* gui = new GUI();

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return 1;
    }

    addrinfo info;

    ZeroMemory(&info, sizeof(info));

    info.ai_flags = AI_PASSIVE;
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_STREAM;
    info.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;

    if (getaddrinfo(nullptr, "4242", &info, &result) != 0)
    {
        WSACleanup();

        return 1;
    }

    SOCKET host = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (host == INVALID_SOCKET)
    {
        freeaddrinfo(result);

        WSACleanup();

        return 1;
    }

    if (bind(host, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
    {
        closesocket(host);

        freeaddrinfo(result);

        WSACleanup();

        return 1;
    }

    freeaddrinfo(result);

    if (listen(host, 1) == SOCKET_ERROR)
    {
        closesocket(host);

        WSACleanup();

        return 1;
    }

    SOCKET client = accept(host, nullptr, nullptr);

    if (client == INVALID_SOCKET)
    {
        closesocket(host);

        WSACleanup();

        return 1;
    }

    closesocket(host);

    if (send(client, data.c_str(), data.size(), 0) == SOCKET_ERROR)
    {
        closesocket(client);

        WSACleanup();

        return 1;
    }

    if (shutdown(client, SD_SEND) == SOCKET_ERROR)
    {
        closesocket(client);

        WSACleanup();

        return 1;
    }

    closesocket(client);

    WSACleanup();

    return 0;
}

#else

int main(int argc, char** argv)
{
    init(argc - 1, argv + 1);

    return 0;
}

#endif
