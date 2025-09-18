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
    return false;
}

DataArray* WindowsServiceManager::readMessage(const MessageType file)
{
    return nullptr;
}

bool WindowsServiceManager::openMessageFiles()
{
    return false;
}

bool WindowsServiceManager::openSemaphores()
{
    return false;
}

bool WindowsServiceManager::clearMessage(const MessageType file)
{
    return false;
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
