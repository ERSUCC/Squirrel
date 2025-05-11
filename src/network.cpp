#include "../include/network.h"

#ifdef _WIN32

//

#else

void Socket::beginBroadcast(const std::function<void(const std::string)> handle)
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
        handle("Failed to bind listening end of socket.");

        return;
    }

    broadcastThread = std::thread([=]()
    {
        sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = 4242;
        addr.sin_addr.s_addr = INADDR_BROADCAST;

        if (sendto(socketHandle, "hello", 6, 0, (sockaddr*)&addr, sizeof(addr)) != 6)
        {
            handle("Failed to broadcast message.");

            return;
        }

        char data[7];

        while (recv(socketHandle, data, 7, 0) != -1)
        {
            std::cout << "response: " << data << "\n";
        }

        shutdown(socketHandle, SHUT_RDWR);

        socketHandle = -1;
    });
}

void Socket::beginListen(const std::function<void(const std::string)> handle)
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
        handle("Failed to bind listening end of socket.");

        return;
    }

    ifaddrs* addrs;

    if (getifaddrs(&addrs) != 0)
    {
        handle("Failed to get network interfaces.");

        return;
    }

    const in_addr_t in_addr = ((sockaddr_in*)addrs->ifa_addr)->sin_addr.s_addr;

    freeifaddrs(addrs);

    broadcastThread = std::thread([&]()
    {
        char data[7];

        if (recvfrom(socketHandle, data, 7, 0, (sockaddr*)&addr, nullptr) == -1)
        {
            handle("Failed to receive broadcast.");

            return;
        }

        shutdown(socketHandle, SHUT_RDWR);

        socketHandle = socket(AF_INET, SOCK_DGRAM, 0);

        if (socketHandle == -1)
        {
            handle("Failed to create socket.");

            return;
        }

        if (sendto(socketHandle, &in_addr, sizeof(in_addr), 0, (sockaddr*)&addr, sizeof(addr)) != sizeof(in_addr))
        {
            handle("Failed to respond to broadcast.");

            return;
        }

        shutdown(socketHandle, SHUT_RDWR);

        socketHandle = -1;
    });
}

#endif
