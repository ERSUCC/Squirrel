#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "json.h"

#define BUFFER_SIZE 512

struct Socket
{
    void beginBroadcast(const std::function<void(const std::string)> handle);
    void beginListen(const std::function<void(const std::string)> handle);

protected:
    virtual bool isBound() const = 0;
    virtual bool createSocket() = 0;
    virtual bool enableBroadcast() = 0;
    virtual bool bindSocket(const unsigned long address, const unsigned int port) = 0;

    virtual std::string getAddress() const = 0;

    virtual bool sendTo(const Message* data, const unsigned long address, const unsigned int port) const = 0;

    virtual Message* receive() const = 0;

    virtual bool destroySocket() = 0;

private:
    std::thread broadcastThread;

};

#ifdef _WIN32

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

struct WinSocket : public Socket
{
    WinSocket();
    ~WinSocket();

protected:
    bool isBound() const override;
    bool createSocket() override;
    bool enableBroadcast() override;
    bool bindSocket(const unsigned long address, const unsigned int port) override;

    std::string getAddress() const override;

    bool sendTo(const Message* message, const unsigned long address, const unsigned int port) const override;

    Message* receive() const override;

    bool destroySocket() override;

private:
    SOCKET socketHandle = INVALID_SOCKET;

};

#else

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>

struct BSDSocket : public Socket
{

protected:
    bool isBound() const override;
    bool createSocket() override;
    bool enableBroadcast() override;
    bool bindSocket(const unsigned long address, const unsigned int port) override;

    std::string getAddress() const override;

    bool sendTo(const Message* message, const unsigned long address, const unsigned int port) const override;

    Message* receive() const override;

    bool destroySocket() override;

private:
    int socketHandle = -1;

};

#endif
