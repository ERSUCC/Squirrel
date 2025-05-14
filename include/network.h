#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <thread>

#define BUFFER_SIZE 128

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

    virtual bool sendTo(const std::string data, const unsigned long address, const unsigned int port) const = 0;

    virtual std::string receive() const = 0;

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

    bool sendTo(const std::string data, const unsigned long address, const unsigned int port) const override;

    std::string receive() const override;

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

    bool sendTo(const std::string data, const unsigned long address, const unsigned int port) const override;

    std::string receive() const override;

    bool destroySocket() override;

private:
    int socketHandle = -1;

};

#endif
