#include "../include/network.h"

#ifdef _WIN32

WinSocket::WinSocket()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "Failed to initialize WinSock.\n";
    }
}

WinSocket::~WinSocket()
{
    WSACleanup();
}

void WinSocket::beginBroadcast(const std::function<void(const std::string)> handle)
{
    if (socketHandle != INVALID_SOCKET)
    {
        handle("Socket is already bound.");

        return;
    }

    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    if (socketHandle == INVALID_SOCKET)
    {
        handle("Failed to create socket.");

        return;
    }

    bool val = true;

    if (setsockopt(socketHandle, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof(bool)) == SOCKET_ERROR)
    {
        closesocket(socketHandle);

        handle("Failed to enable socket broadcast.");

        return;
    }

    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = 4242;
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(socketHandle);

        handle("Failed to bind socket.");

        return;
    }

    unsigned long size = sizeof(IP_ADAPTER_ADDRESSES) * 32;

    IP_ADAPTER_ADDRESSES* addrs = (IP_ADAPTER_ADDRESSES*)malloc(size);

    if (GetAdaptersAddresses(AF_INET, 0, nullptr, addrs, &size) != NO_ERROR)
    {
        free(addrs);

        handle("Failed to get network interfaces.");

        return;
    }

    IP_ADAPTER_ADDRESSES* head = addrs;

    while (addrs)
    {
        if (addrs->IfType != IF_TYPE_SOFTWARE_LOOPBACK && addrs->OperStatus == IfOperStatusUp)
        {
            break;
        }

        addrs = addrs->Next;
    }

    if (!addrs)
    {
        free(head);

        handle("Failed to find a valid network interface");

        return;
    }

    const char* addrStr = inet_ntoa(((sockaddr_in*)addrs->FirstUnicastAddress->Address.lpSockaddr)->sin_addr);

    const unsigned int addrLen = strnlen(addrStr, 15);

    free(head);

    broadcastThread = std::thread([=]()
    {
        sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = 4242;
        addr.sin_addr.s_addr = INADDR_BROADCAST;

        if (sendto(socketHandle, addrStr, addrLen, 0, (sockaddr*)&addr, sizeof(addr)) != addrLen)
        {
            handle("Failed to broadcast message.");

            return;
        }

        char data[128];

        int n;

        do
        {
            n = recv(socketHandle, data, 128, 0);

            if (n != -1)
            {
                data[n] = '\0';

                std::cout << "response: " << data << "\n";
            }
        } while (n != -1);

        shutdown(socketHandle, SD_BOTH);
        closesocket(socketHandle);
    });
}

void WinSocket::beginListen(const std::function<void(const std::string)> handle)
{
    if (socketHandle != INVALID_SOCKET)
    {
        handle("Socket is already bound.");

        return;
    }

    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    if (socketHandle == INVALID_SOCKET)
    {
        handle("Failed to create socket.");

        return;
    }

    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = 4242;
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(socketHandle);

        handle("Failed to bind socket.");

        return;
    }

    unsigned long size = sizeof(IP_ADAPTER_ADDRESSES) * 32;

    IP_ADAPTER_ADDRESSES* addrs = (IP_ADAPTER_ADDRESSES*)malloc(size);

    if (GetAdaptersAddresses(AF_INET, 0, nullptr, addrs, &size) != NO_ERROR)
    {
        free(addrs);

        handle("Failed to get network interfaces.");

        return;
    }

    IP_ADAPTER_ADDRESSES* head = addrs;

    while (addrs)
    {
        if (addrs->IfType != IF_TYPE_SOFTWARE_LOOPBACK && addrs->OperStatus == IfOperStatusUp)
        {
            break;
        }

        addrs = addrs->Next;
    }

    if (!addrs)
    {
        free(head);

        handle("Failed to find a valid network interface");

        return;
    }

    const char* addrStr = inet_ntoa(((sockaddr_in*)addrs->FirstUnicastAddress->Address.lpSockaddr)->sin_addr);

    const unsigned int addrLen = strnlen(addrStr, 15);

    free(head);

    broadcastThread = std::thread([=]()
    {
        char data[128];

        int n = recv(socketHandle, data, 128, 0);

        if (n == -1)
        {
            handle("Failed to receive broadcast.");

            return;
        }

        data[n] = '\0';

        sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = 4242;
        addr.sin_addr.s_addr = inet_addr(data);

        if (sendto(socketHandle, addrStr, addrLen, 0, (sockaddr*)&addr, sizeof(addr)) != addrLen)
        {
            handle("Failed to respond to broadcast.");

            return;
        }
    });
}

#else

void BSDSocket::beginBroadcast(const std::function<void(const std::string)> handle)
{
    if (socketHandle != -1)
    {
        handle("Socket is alread bound.");

        return;
    }

    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    if (socketHandle == -1)
    {
        handle("Failed to create socket.");

        return;
    }

    int val = 1;

    if (setsockopt(socketHandle, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) != 0)
    {
        handle("Failed to enable socket broadcast.");

        return;
    }

    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = 4242;
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) != 0)
    {
        handle("Failed to bind socket.");

        return;
    }

    ifaddrs* addrs;

    if (getifaddrs(&addrs) != 0)
    {
        handle("Failed to get network interfaces.");

        return;
    }

    ifaddrs* head = addrs;

    while (addrs)
    {
        if (addrs->ifa_addr->sa_family == AF_INET && ((sockaddr_in*)addrs->ifa_addr)->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
        {
            break;
        }

        addrs = addrs->ifa_next;
    }

    if (!addrs)
    {
        freeifaddrs(head);

        handle("Failed to find a valid network interface.");

        return;
    }

    const char* addrStr = inet_ntoa(((sockaddr_in*)addrs->ifa_addr)->sin_addr);

    const unsigned int addrLen = strnlen(addrStr, 15);

    freeifaddrs(head);

    broadcastThread = std::thread([=]()
    {
        sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = 4242;
        addr.sin_addr.s_addr = INADDR_BROADCAST;

        if (sendto(socketHandle, addrStr, addrLen, 0, (sockaddr*)&addr, sizeof(addr)) != addrLen)
        {
            handle("Failed to broadcast message.");

            return;
        }

        char data[128];

        int n;

        do
        {
            n = recv(socketHandle, data, 128, 0);

            if (n != -1)
            {
                data[n] = '\0';

                std::cout << "response: " << data << "\n";
            }
        } while (n != -1);

        shutdown(socketHandle, SHUT_RDWR);

        socketHandle = -1;
    });
}

void BSDSocket::beginListen(const std::function<void(const std::string)> handle)
{
    if (socketHandle != -1)
    {
        handle("Socket is already bound.");

        return;
    }

    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    if (socketHandle == -1)
    {
        handle("Failed to create socket.");

        return;
    }

    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = 4242;
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) != 0)
    {
        handle("Failed to bind socket.");

        return;
    }

    ifaddrs* addrs;

    if (getifaddrs(&addrs) != 0)
    {
        handle("Failed to get network interfaces.");

        return;
    }

    ifaddrs* head = addrs;

    while (addrs)
    {
        if (addrs->ifa_addr->sa_family == AF_INET && ((sockaddr_in*)addrs->ifa_addr)->sin_addr.s_addr != htonl(INADDR_LOOPBACK))
        {
            break;
        }

        addrs = addrs->ifa_next;
    }

    if (!addrs)
    {
        freeifaddrs(head);

        handle("Failed to find a valid network interface.");

        return;
    }

    const char* addrStr = inet_ntoa(((sockaddr_in*)addrs->ifa_addr)->sin_addr);

    const unsigned int addrLen = strnlen(addrStr, 15);

    freeifaddrs(head);

    broadcastThread = std::thread([=]()
    {
        char data[128];

        int n = recv(socketHandle, data, 128, 0);

        if (n == -1)
        {
            handle("Failed to receive broadcast.");

            return;
        }

        data[n] = '\0';

        sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = 4242;
        addr.sin_addr.s_addr = inet_addr(data);

        if (sendto(socketHandle, addrStr, addrLen, 0, (sockaddr*)&addr, sizeof(addr)) != addrLen)
        {
            handle("Failed to respond to broadcast.");

            return;
        }

        shutdown(socketHandle, SHUT_RDWR);

        socketHandle = -1;
    });
}

#endif
