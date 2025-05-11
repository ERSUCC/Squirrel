#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32

#include <WinSock2.h>
#include <WS2tcpip.h>

struct Socket
{
};

#else

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>

struct Socket
{
    void beginBroadcast(const std::function<void(const std::string)> handle);
    void beginListen(const std::function<void(const std::string)> handle);

private:
    int socketHandle = -1;

    std::thread broadcastThread;

};

#endif
