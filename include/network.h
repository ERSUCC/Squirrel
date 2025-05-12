#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <thread>

struct Socket
{
    virtual void beginBroadcast(const std::function<void(const std::string)> handle) = 0;
    virtual void beginListen(const std::function<void(const std::string)> handle) = 0;
};

#ifdef _WIN32

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

struct WinSocket : public Socket
{
    WinSocket();
    ~WinSocket();

    void beginBroadcast(const std::function<void(const std::string)> handle);
    void beginListen(const std::function<void(const std::string)> handle);

private:
    SOCKET socketHandle = INVALID_SOCKET;

    std::thread broadcastThread;

};

#else

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>

struct BSDSocket : public Socket
{
    void beginBroadcast(const std::function<void(const std::string)> handle);
    void beginListen(const std::function<void(const std::string)> handle);

private:
    int socketHandle = -1;

    std::thread broadcastThread;

};

#endif
