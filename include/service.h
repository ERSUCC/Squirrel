#pragma once

#include <functional>
#include <string>
#include <thread>

#include "errors.h"
#include "network.h"

struct DataArray
{
    DataArray(char* data, const size_t length);
    ~DataArray();

    char* data;

    const size_t length;
};

enum MessageType
{
    Service,
    Application
};

struct MessageConstants
{
    static constexpr unsigned char RESPONSE = 0x00;
    static constexpr unsigned char CONNECTION = 0x01;
};

struct ServiceManager
{
    ServiceManager(ErrorHandler* errorHandler, NetworkManager* networkManager);

    bool startService();
    bool connectService(const std::function<void(const std::string, const std::string)> handleResponse);

    virtual bool writeMessage(const MessageType file, const DataArray* data) = 0;

    virtual DataArray* readMessage(const MessageType file) = 0;

protected:
    bool initialize();

    virtual bool openMessageFiles() = 0;
    virtual bool openSemaphores() = 0;

    virtual bool clearMessage(const MessageType file) = 0;

    ErrorHandler* errorHandler;
    NetworkManager* networkManager;

    std::thread listenThread;

};

#ifdef _WIN32

#include <fileapi.h>

struct WindowsServiceManager : public ServiceManager
{
    WindowsServiceManager(ErrorHandler* errorHandler, NetworkManager* networkManager);

    bool writeMessage(const MessageType file, const DataArray* data) override;

    DataArray* readMessage(const MessageType file) override;

protected:
    bool openMessageFiles() override;
    bool openSemaphores() override;

    bool clearMessage(const MessageType file) override;

private:
    HANDLE serviceFile;
    HANDLE applicationFile;

    HANDLE serviceSem;
    HANDLE applicationSem;

};

#else

#include <sys/file.h>
#include <semaphore.h>
#include <unistd.h>

struct POSIXServiceManager : public ServiceManager
{
    POSIXServiceManager(ErrorHandler* ErrorHandler, NetworkManager* networkManager);

    bool writeMessage(const MessageType file, const DataArray* data) override;

    DataArray* readMessage(const MessageType file) override;

protected:
    bool openMessageFiles() override;
    bool openSemaphores() override;

    bool clearMessage(const MessageType file) override;

private:
    int serviceFile;
    int applicationFile;

    sem_t* serviceSem;
    sem_t* applicationSem;

};

#endif
