#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "base64.h"
#include "json.h"

#define UDP_PORT 4242
#define TCP_PORT 4244

#define BUFFER_SIZE 512

struct UDPSocket
{
    virtual bool create() = 0;
    virtual bool socketBind(const std::string address, const unsigned int port) const = 0;
    virtual bool socketSend(const Message* message, const std::string address, const unsigned int port) const = 0;

    virtual Message* receive() const = 0;

    virtual bool destroy() = 0;
};

struct TCPSocket
{
    virtual bool create() = 0;
    virtual bool socketBind(const std::string address, const unsigned int port) const = 0;
    virtual bool socketConnect(const std::string address, const unsigned int port) const = 0;
    virtual bool socketListen() const = 0;
    virtual bool socketAccept() = 0;
    virtual bool socketSend(const Message* message) const = 0;

    virtual Message* receive() const = 0;

    virtual bool destroy() = 0;
};

struct NetworkManager
{
    NetworkManager(const std::string address);

    void beginBroadcast(const std::function<void(const std::string)> handleResponse, const std::function<void(const std::string)> handleError);
    void beginListen(const std::function<void(const std::string, const std::string&)> handleReceive, const std::function<void(const std::string)> handleError);
    void beginTransfer(const std::filesystem::path path, const std::string ip, const std::function<void(const std::string)> handleError);
    void beginReceive(const std::string ip, const std::function<void(const std::string, const std::string&)> handleReceive, const std::function<void(const std::string)> handleError);

protected:
    virtual UDPSocket* newUDPSocket() const = 0;
    virtual TCPSocket* newTCPSocket() const = 0;

    virtual std::string getAddress() const = 0;

private:
    const std::string address;

    UDPSocket* udpSocket = nullptr;
    TCPSocket* tcpSocket = nullptr;

    std::thread broadcastThread;
    std::thread responseThread;
    std::thread listenThread;
    std::thread transferThread;

};

#ifdef _WIN32

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

struct WinUDPSocket : public UDPSocket
{
    bool create() override;
    bool socketBind(const std::string address, const unsigned int port) const override;
    bool socketSend(const Message* message, const std::string address, const unsigned int port) const override;

    Message* receive() const override;

    bool destroy() override;

private:
    SOCKET socketHandle = INVALID_SOCKET;

};

struct WinTCPSocket : public TCPSocket
{
    bool create() override;
    bool socketBind(const std::string address, const unsigned int port) const override;
    bool socketConnect(const std::string address, const unsigned int port) const override;
    bool socketListen() const override;
    bool socketAccept() override;
    bool socketSend(const Message* message) const override;

    Message* receive() const override;

    bool destroy() override;

private:
    SOCKET socketHandle = INVALID_SOCKET;

};

struct WinNetworkManager : public NetworkManager
{
    WinNetworkManager();
    ~WinNetworkManager();

protected:
    UDPSocket* newUDPSocket() const override;
    TCPSocket* newTCPSocket() const override;

    std::string getAddress() const override;

};

#else

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>

struct BSDUDPSocket : public UDPSocket
{
    bool create() override;
    bool socketBind(const std::string address, const unsigned int port) const override;
    bool socketSend(const Message* message, const std::string address, const unsigned int port) const override;

    Message* receive() const override;

    bool destroy() override;

private:
    int socketHandle = -1;

};

struct BSDTCPSocket : public TCPSocket
{
    bool create() override;
    bool socketBind(const std::string address, const unsigned int port) const override;
    bool socketConnect(const std::string address, const unsigned int port) const override;
    bool socketListen() const override;
    bool socketAccept() override;
    bool socketSend(const Message* message) const override;

    Message* receive() const override;

    bool destroy() override;

private:
    int socketHandle = -1;

};

struct BSDNetworkManager : public NetworkManager
{
    BSDNetworkManager();

protected:
    UDPSocket* newUDPSocket() const override;
    TCPSocket* newTCPSocket() const override;

    std::string getAddress() const override;

};

#endif
