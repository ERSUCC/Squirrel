#include "../include/network.h"

void Socket::beginBroadcast(const std::function<void(const std::string)> handle)
{
    if (isBound())
    {
        handle("Socket is already bound.");

        return;
    }

    if (!createSocket())
    {
        handle("Failed to create socket.");

        return;
    }

    if (!enableBroadcast())
    {
        handle("Failed to enable socket broadcast.");

        return;
    }

    if (!bindSocket(INADDR_ANY, 4242))
    {
        handle("Failed to bind socket.");

        return;
    }

    const std::string address = getAddress();

    if (address.empty())
    {
        handle("Failed to find a valid network interface.");

        return;
    }

    broadcastThread = std::thread([=]()
    {
        const Message* broadcastMessage = new Message(new JSONObject({ { "ip", new JSONString(address) } }));

        if (!sendTo(broadcastMessage, INADDR_BROADCAST, 4242))
        {
            handle("Failed to broadcast message.");

            return;
        }

        Message* message;

        do
        {
            message = receive();

            if (message)
            {
                std::stringstream stream;

                message->serialize(stream);

                std::cout << "Received: " << stream.str() << "\n";
            }
        } while (message);

        if (!destroySocket())
        {
            handle("Failed to close socket.");

            return;
        }
    });
}

void Socket::beginListen(const std::function<void(const std::string)> handle)
{
    if (isBound())
    {
        handle("Socket is already bound.");

        return;
    }

    if (!createSocket())
    {
        handle("Failed to create socket.");

        return;
    }

    if (!bindSocket(INADDR_ANY, 4242))
    {
        handle("Failed to bind socket.");

        return;
    }

    const std::string address = getAddress();

    broadcastThread = std::thread([=]()
    {
        const Message* message = receive();

        if (!message)
        {
            handle("Failed to receive broadcast.");

            return;
        }

        const std::optional<std::string> ip = message->data->getProperty("ip")->asString();

        if (!ip)
        {
            handle("Failed to receive broadcast.");

            return;
        }

        if (!sendTo(message, inet_addr(ip.value().c_str()), 4242))
        {
            handle("Failed to respond to broadcast.");

            return;
        }
    });
}

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

bool WinSocket::isBound() const
{
    return socketHandle != INVALID_SOCKET;
}

bool WinSocket::createSocket()
{
    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    return socketHandle != INVALID_SOCKET;
}

bool WinSocket::enableBroadcast()
{
    bool val = true;

    return setsockopt(socketHandle, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof(bool)) != SOCKET_ERROR;
}

bool WinSocket::bindSocket(const unsigned long address, const unsigned int port)
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = address;

    return bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
}

std::string WinSocket::getAddress() const
{
    unsigned long size = sizeof(IP_ADAPTER_ADDRESSES) * 32;

    IP_ADAPTER_ADDRESSES* addrs = (IP_ADAPTER_ADDRESSES*)malloc(size);

    if (GetAdaptersAddresses(AF_INET, 0, nullptr, addrs, &size) != NO_ERROR)
    {
        free(addrs);

        return "";
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

        return "";
    }

    char* address = inet_ntoa(((sockaddr_in*)addrs->FirstUnicastAddress->Address.lpSockaddr)->sin_addr);

    free(head);

    return address;
}

bool WinSocket::sendTo(const Message* message, const unsigned long address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = address;

    std::stringstream stream;

    message->serialize(stream);

    const std::string& str = stream.str();

    return sendto(socketHandle, str.c_str(), str.size() + 1, 0, (sockaddr*)&addr, sizeof(addr)) == str.size() + 1;
}

Message* WinSocket::receive() const
{
    char buffer[BUFFER_SIZE];

    if (recv(socketHandle, buffer, BUFFER_SIZE, 0) == -1)
    {
        return nullptr;
    }

    buffer[BUFFER_SIZE - 1] = '\0';

    return Message::deserialize(std::stringstream(buffer));
}

bool WinSocket::destroySocket()
{
    if (shutdown(socketHandle, SD_BOTH) == SOCKET_ERROR)
    {
        return false;
    }

    if (closesocket(socketHandle) == SOCKET_ERROR)
    {
        return false;
    }

    socketHandle = INVALID_SOCKET;

    return true;
}

#else

bool BSDSocket::isBound() const
{
    return socketHandle != -1;
}

bool BSDSocket::createSocket()
{
    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    return socketHandle != -1;
}

bool BSDSocket::enableBroadcast()
{
    int val = 1;

    return setsockopt(socketHandle, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) == 0;
}

bool BSDSocket::bindSocket(const unsigned long address, const unsigned int port)
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = address;

    return bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) == 0;
}

std::string BSDSocket::getAddress() const
{
    ifaddrs* addrs;

    if (getifaddrs(&addrs) != 0)
    {
        return "";
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

        return "";
    }

    const char* addrStr = inet_ntoa(((sockaddr_in*)addrs->ifa_addr)->sin_addr);

    freeifaddrs(head);

    return addrStr;
}

bool BSDSocket::sendTo(const Message* message, const unsigned long address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = address;

    std::stringstream stream;

    message->serialize(stream);

    const std::string& str = stream.str();

    return sendto(socketHandle, str.c_str(), str.size() + 1, 0, (sockaddr*)&addr, sizeof(addr)) == str.size() + 1;
}

Message* BSDSocket::receive() const
{
    char buffer[BUFFER_SIZE];

    if (recv(socketHandle, buffer, BUFFER_SIZE, 0) == -1)
    {
        return nullptr;
    }

    buffer[BUFFER_SIZE - 1] = '\0';

    return Message::deserialize(std::stringstream(buffer));
}

bool BSDSocket::destroySocket()
{
    if (shutdown(socketHandle, SHUT_RDWR) != 0)
    {
        return false;
    }

    socketHandle = -1;

    return true;
}

#endif
