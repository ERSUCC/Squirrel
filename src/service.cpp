#include "../include/service.h"

DataArray::DataArray(char* data, const size_t length) :
    data(data), length(length) {}

DataArray::~DataArray()
{
    free(data);
}

ServiceManager::ServiceManager(ErrorHandler* errorHandler, NetworkManager* networkManager) :
    errorHandler(errorHandler), networkManager(networkManager) {}

bool ServiceManager::startService()
{
    if (!initialize())
    {
        return false;
    }

    listenThread = std::thread([=]()
    {
        while (true)
        {
            if (const DataArray* data = readMessage(MessageType::Service))
            {
                if (data->data[0] == MessageConstants::CONNECTION)
                {
                    networkManager->beginConnect(networkManager->convertAddress(*(uint32_t*)(data->data + 4)));
                }
            }
        }
    });

    return true;
}

bool ServiceManager::connectService(const std::function<void(const std::string, const std::string)> handleResponse)
{
    if (!initialize())
    {
        return false;
    }

    listenThread = std::thread([=]()
    {
        while (true)
        {
            if (const DataArray* data = readMessage(MessageType::Application))
            {
                if (data->data[0] == MessageConstants::RESPONSE)
                {
                    handleResponse(data->data + 8, networkManager->convertAddress(*(uint32_t*)(data->data + 4)));
                }
            }
        }
    });

    return true;
}

bool ServiceManager::initialize()
{
    if (!openMessageFiles())
    {
        errorHandler->push(SquirrelFileException("Error opening message files."));

        return false;
    }

    if (!openSemaphores())
    {
        errorHandler->push(SquirrelFileException("Error opening message semaphores."));

        return false;
    }

    if (!clearMessage(MessageType::Service))
    {
        return false;
    }

    if (!clearMessage(MessageType::Application))
    {
        return false;
    }

    return true;
}

#ifdef _WIN32

WindowsServiceManager::WindowsServiceManager(ErrorHandler* errorHandler, NetworkManager* networkManager) :
    ServiceManager(errorHandler, networkManager) {}

bool WindowsServiceManager::writeMessage(const MessageType file, const DataArray* data)
{
    HANDLE messageFile;
    HANDLE sem;

    switch (file)
    {
        case MessageType::Service:
            messageFile = serviceFile;
            sem = serviceSem;

            break;

        case MessageType::Application:
            messageFile = applicationFile;
            sem = applicationSem;

            break;
    }

    DWORD size = GetFileSize(messageFile, nullptr) + data->length;

    OVERLAPPED overlapped;

    overlapped.hEvent = nullptr;

    if (!LockFileEx(messageFile, LOCKFILE_EXCLUSIVE_LOCK, 0, LOWORD(size), HIWORD(size), &overlapped))
    {
        errorHandler->push(SquirrelFileException("Error acquiring file lock."));

        return false;
    }

    if (SetFilePointer(messageFile, 0, nullptr, FILE_END) == INVALID_SET_FILE_POINTER)
    {
        errorHandler->push(SquirrelFileException("Error moving to end of file."));

        return nullptr;
    }

    DWORD result;

    if (!WriteFile(messageFile, data->data, data->length, &result, nullptr))
    {
        errorHandler->push(SquirrelFileException("Error writing to message file."));

        return false;
    }

    if (!UnlockFileEx(messageFile, 0, LOWORD(size), HIWORD(size), &overlapped))
    {
        errorHandler->push(SquirrelFileException("Error releasing file lock."));

        return false;
    }

    if (!ReleaseSemaphore(sem, 1, nullptr))
    {
        errorHandler->push(SquirrelFileException("Error posting to semaphore."));

        return false;
    }

    return true;
}

DataArray* WindowsServiceManager::readMessage(const MessageType file)
{
    HANDLE messageFile;
    HANDLE sem;

    switch (file)
    {
        case MessageType::Service:
            messageFile = serviceFile;
            sem = serviceSem;

            break;

        case MessageType::Application:
            messageFile = applicationFile;
            sem = applicationSem;

            break;
    }

    if (WaitForSingleObject(sem, INFINITE) != WAIT_OBJECT_0)
    {
        errorHandler->push(SquirrelFileException("Error waiting for semaphore."));

        return nullptr;
    }

    DWORD size = GetFileSize(messageFile, nullptr);

    OVERLAPPED overlapped;

    overlapped.hEvent = nullptr;

    if (!LockFileEx(messageFile, LOCKFILE_EXCLUSIVE_LOCK, 0, LOWORD(size), HIWORD(size), &overlapped))
    {
        errorHandler->push(SquirrelFileException("Error acquiring file lock."));

        return nullptr;
    }

    if (SetFilePointer(messageFile, 0, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        errorHandler->push(SquirrelFileException("Error returning to start of file."));

        return nullptr;
    }

    char* data = (char*)malloc(sizeof(char) * 4);

    DWORD result;

    if (!ReadFile(messageFile, data, sizeof(char) * 4, &result, nullptr))
    {
        errorHandler->push(SquirrelFileException("Error reading from message file."));

        return nullptr;
    }

    const uint16_t length = *(uint16_t*)(data + 2);

    data = (char*)realloc(data, sizeof(char) * length);

    if (!ReadFile(messageFile, data + 4, sizeof(char) * (length - 4), &result, nullptr))
    {
        errorHandler->push(SquirrelFileException("Error reading from message file."));

        return nullptr;
    }

    size_t block = 1024;
    size_t max = block;
    size_t current = 0;

    char* buffer = (char*)malloc(sizeof(char) * max);

    do
    {
        if (ReadFile(messageFile, buffer + current, sizeof(char) * block, &result, nullptr))
        {
            current += result;

            if (max - current < block)
            {
                max *= 2;

                buffer = (char*)realloc(buffer, max);
            }
        }
    } while (result > 0);

    if (SetFilePointer(messageFile, 0, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER || !SetEndOfFile(messageFile))
    {
        errorHandler->push(SquirrelFileException("Error clearing message file contents."));

        return nullptr;
    }

    if (!WriteFile(messageFile, buffer, sizeof(char) * current, &result, nullptr))
    {
        errorHandler->push(SquirrelFileException("Error writing to message file."));

        return nullptr;
    }

    if (!UnlockFileEx(messageFile, 0, LOWORD(size), HIWORD(size), &overlapped))
    {
        errorHandler->push(SquirrelFileException("Error releasing file lock."));

        return nullptr;
    }

    return new DataArray(data, length);
}

bool WindowsServiceManager::openMessageFiles()
{
    LPSTR path = (LPSTR)malloc(sizeof(char) * (MAX_PATH + 13));

    const DWORD length = GetTempPath(MAX_PATH, path);

    if (length == 0)
    {
        free(path);

        return false;
    }

    strncpy(path + length, "sqrl_msg_svc", 12);

    path[length + 12] = '\0';

    serviceFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    strncpy(path + length, "sqrl_msg_app", 12);

    applicationFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    return serviceFile && applicationFile;
}

bool WindowsServiceManager::openSemaphores()
{
    serviceSem = CreateSemaphore(nullptr, 0, LONG_MAX, "sqrl_sem_svc");
    applicationSem = CreateSemaphore(nullptr, 0, LONG_MAX, "sqrl_sem_app");

    return serviceSem && applicationSem;
}

bool WindowsServiceManager::clearMessage(const MessageType file)
{
    HANDLE messageFile;
    HANDLE sem;

    switch (file)
    {
        case MessageType::Service:
            messageFile = serviceFile;
            sem = serviceSem;

            break;

        case MessageType::Application:
            messageFile = applicationFile;
            sem = applicationSem;

            break;
    }

    while (WaitForSingleObject(sem, 0) != WAIT_TIMEOUT) {}

    DWORD size = GetFileSize(messageFile, nullptr);

    OVERLAPPED overlapped;

    overlapped.hEvent = nullptr;

    if (!LockFileEx(messageFile, LOCKFILE_EXCLUSIVE_LOCK, 0, LOWORD(size), HIWORD(size), &overlapped))
    {
        errorHandler->push(SquirrelFileException("Error acquiring file lock."));

        return false;
    }

    if (SetFilePointer(messageFile, 0, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER || !SetEndOfFile(messageFile))
    {
        errorHandler->push(SquirrelFileException("Error clearing message file contents."));

        return false;
    }

    if (!UnlockFileEx(messageFile, 0, LOWORD(size), HIWORD(size), &overlapped))
    {
        errorHandler->push(SquirrelFileException("Error releasing file lock."));

        return false;
    }

    return true;
}

#else

POSIXServiceManager::POSIXServiceManager(ErrorHandler* errorHandler, NetworkManager* networkManager) :
    ServiceManager(errorHandler, networkManager) {}

bool POSIXServiceManager::writeMessage(const MessageType file, const DataArray* data)
{
    int messageFile;

    sem_t* sem;

    switch (file)
    {
        case MessageType::Service:
            messageFile = serviceFile;
            sem = serviceSem;

            break;

        case MessageType::Application:
            messageFile = applicationFile;
            sem = applicationSem;

            break;
    }

    if (flock(messageFile, LOCK_EX) == -1)
    {
        errorHandler->push(SquirrelFileException("Error acquiring file lock."));

        return false;
    }

    const ssize_t result = write(messageFile, data->data, data->length);

    if (result == -1)
    {
        errorHandler->push(SquirrelFileException("Error writing to message file."));

        return false;
    }

    if (flock(messageFile, LOCK_UN) == -1)
    {
        errorHandler->push(SquirrelFileException("Error releasing file lock."));

        return false;
    }

    if (sem_post(sem) == -1)
    {
        errorHandler->push(SquirrelFileException("Error posting to semaphore."));

        return false;
    }

    return true;
}

DataArray* POSIXServiceManager::readMessage(const MessageType file)
{
    int messageFile;

    sem_t* sem;

    switch (file)
    {
        case MessageType::Service:
            messageFile = serviceFile;
            sem = serviceSem;

            break;

        case MessageType::Application:
            messageFile = applicationFile;
            sem = applicationSem;

            break;
    }

    if (sem_wait(sem) == -1)
    {
        errorHandler->push(SquirrelFileException("Error waiting for semaphore."));

        return nullptr;
    }

    if (flock(messageFile, LOCK_EX) == -1)
    {
        errorHandler->push(SquirrelFileException("Error acquiring file lock."));

        return nullptr;
    }

    if (lseek(messageFile, 0, SEEK_SET) == -1)
    {
        errorHandler->push(SquirrelFileException("Error returning to start of file."));

        return nullptr;
    }

    char* data = (char*)malloc(sizeof(char) * 4);

    if (read(messageFile, data, sizeof(char) * 4) == -1)
    {
        errorHandler->push(SquirrelFileException("Error reading from message file."));

        return nullptr;
    }

    const uint16_t length = *(uint16_t*)(data + 2);

    data = (char*)realloc(data, sizeof(char) * length);

    if (read(messageFile, data + 4, sizeof(char) * (length - 4)) == -1)
    {
        errorHandler->push(SquirrelFileException("Error reading from message file."));

        return nullptr;
    }

    size_t block = 1024;
    size_t max = block;
    size_t current = 0;

    char* buffer = (char*)malloc(sizeof(char) * max);

    ssize_t result;

    do
    {
        result = read(messageFile, buffer + current, sizeof(char) * block);

        if (result > 0)
        {
            current += result;

            if (max - current < block)
            {
                max *= 2;

                buffer = (char*)realloc(buffer, max);
            }
        }
    } while (result > 0);

    if (ftruncate(messageFile, 0) == -1)
    {
        errorHandler->push(SquirrelFileException("Error clearing message file contents."));

        return nullptr;
    }

    if (write(messageFile, buffer, sizeof(char) * current) == -1)
    {
        errorHandler->push(SquirrelFileException("Error writing to message file."));

        return nullptr;
    }

    if (flock(messageFile, LOCK_UN) == -1)
    {
        errorHandler->push(SquirrelFileException("Error releasing file lock."));

        return nullptr;
    }

    return new DataArray(data, length);
}

bool POSIXServiceManager::openMessageFiles()
{
    serviceFile = open("/var/tmp/sqrl_msg_svc", O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
    applicationFile = open("/var/tmp/sqrl_msg_app", O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);

    return serviceFile != -1 && applicationFile != -1;
}

bool POSIXServiceManager::openSemaphores()
{
    serviceSem = sem_open("sqrl_sem_svc", O_CREAT, S_IRUSR | S_IWUSR, 0);
    applicationSem = sem_open("sqrl_sem_app", O_CREAT, S_IRUSR | S_IWUSR, 0);

    return serviceSem && applicationSem;
}

bool POSIXServiceManager::clearMessage(const MessageType file)
{
    int messageFile;

    sem_t* sem;

    switch (file)
    {
        case MessageType::Service:
            messageFile = serviceFile;
            sem = serviceSem;

            break;

        case MessageType::Application:
            messageFile = applicationFile;
            sem = applicationSem;

            break;
    }

    while (sem_trywait(sem) != -1) {}

    if (flock(messageFile, LOCK_EX) == -1)
    {
        errorHandler->push(SquirrelFileException("Error acquiring file lock."));

        return false;
    }

    if (ftruncate(messageFile, 0) == -1)
    {
        errorHandler->push(SquirrelFileException("Error clearing message file contents."));

        return false;
    }

    if (flock(messageFile, LOCK_UN) == -1)
    {
        errorHandler->push(SquirrelFileException("Error releasing file lock."));

        return false;
    }

    return true;
}

#endif
