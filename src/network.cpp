#include "../include/network.h"

void NetworkManager::beginBroadcast(const std::function<void(const std::string)> handleResponse, const std::function<void(const std::string)> handleError)
{
    udpSocket = newUDPSocket();

    if (!udpSocket->create())
    {
        handleError("Failed to create socket.");

        return;
    }

    if (!udpSocket->socketBind("0.0.0.0", UDP_PORT))
    {
        handleError("Failed to bind socket.");

        return;
    }

    const std::string address = getAddress();

    if (address.empty())
    {
        handleError("Failed to find a valid network interface.");

        return;
    }

    broadcastThread = std::thread([=]()
    {
        std::chrono::high_resolution_clock clock;
        std::chrono::time_point<std::chrono::high_resolution_clock> last;

        while (udpSocket)
        {
            if ((clock.now() - last).count() >= 1e9)
            {
                const Message* broadcastMessage = new Message(new JSONObject(
                {
                    { "type", new JSONString("broadcast") },
                    { "ip", new JSONString(address) }
                }));

                if (!udpSocket->socketSend(broadcastMessage, "255.255.255.255", UDP_PORT))
                {
                    handleError("Failed to broadcast message.");

                    return;
                }

                last = clock.now();
            }
        };
    });

    responseThread = std::thread([=]()
    {
        while (const Message* message = udpSocket->receive())
        {
            if (message->data->getProperty("type")->asString().value_or("") == "available")
            {
                if (const std::optional<std::string> ip = message->data->getProperty("ip")->asString())
                {
                    handleResponse(ip.value());
                }
            }
        }
    });
}

void NetworkManager::beginListen(const std::function<void(const std::string)> handleError)
{
    udpSocket = newUDPSocket();

    if (!udpSocket->create())
    {
        handleError("Failed to create socket.");

        return;
    }

    if (!udpSocket->socketBind("0.0.0.0", UDP_PORT))
    {
        handleError("Failed to bind socket.");

        return;
    }

    const std::string address = getAddress();

    if (address.empty())
    {
        handleError("Failed to find a valid network interface.");

        return;
    }

    listenThread = std::thread([=]()
    {
        while (const Message* message = udpSocket->receive())
        {
            if (message->data->getProperty("type")->asString().value_or("") == "broadcast")
            {
                if (const std::optional<std::string> ip = message->data->getProperty("ip")->asString())
                {
                    const Message* response = new Message(new JSONObject(
                    {
                        { "type", new JSONString("available") },
                        { "ip", new JSONString(address) }
                    }));

                    if (!udpSocket->socketSend(response, ip.value(), UDP_PORT))
                    {
                        handleError("Failed to respond to broadcast.");

                        return;
                    }

                    beginReceive(ip.value(), handleError);
                }
            }
        }
    });
}

void NetworkManager::beginTransfer(const std::filesystem::path path, const std::string ip, const std::function<void(const std::string)> handleError)
{
    const std::string address = getAddress();

    if (address.empty())
    {
        handleError("Failed to find a valid network interface.");

        return;
    }

    std::ifstream file(path, std::ios_base::binary);

    if (!file.is_open())
    {
        handleError("Failed to open specified file.");

        return;
    }

    std::stringstream stream;

    stream << file.rdbuf();

    file.close();

    const std::string name = path.filename().string();
    const std::string data = Base64::encode(stream.str());

    const Message* message = new Message(new JSONObject(
    {
        { "type", new JSONString("transfer") },
        { "ip", new JSONString(address) },
        { "name", new JSONString(name) },
        { "data", new JSONString(data) }
    }));

    udpSocket->destroy();

    udpSocket = nullptr;

    broadcastThread.join();

    tcpSocket = newTCPSocket();

    if (!tcpSocket->create())
    {
        handleError("Failed to create socket.");

        return;
    }

    transferThread = std::thread([=]()
    {
        if (!tcpSocket->socketConnect(ip, TCP_PORT))
        {
            handleError("Failed to connect to socket.");

            return;
        }

        if (!tcpSocket->socketSend(message))
        {
            handleError("Failed to transfer file.");

            return;
        }

        if (!tcpSocket->destroy())
        {
            handleError("Failed to destroy socket.");
        }
    });
}

void NetworkManager::beginReceive(const std::string ip, const std::function<void(const std::string)> handleError)
{
    if (tcpSocket)
    {
        return;
    }

    tcpSocket = newTCPSocket();

    if (!tcpSocket->create())
    {
        handleError("Failed to create socket.");

        return;
    }

    const std::string address = getAddress();

    if (address.empty())
    {
        handleError("Failed to find a valid network interface.");

        return;
    }

    if (!tcpSocket->socketBind(address, TCP_PORT))
    {
        handleError("Failed to bind socket.");

        return;
    }

    if (!tcpSocket->socketListen())
    {
        handleError("Failed to listen on socket.");

        return;
    }

    transferThread = std::thread([=]()
    {
        if (!tcpSocket->socketAccept())
        {
            handleError("Failed to accept connection.");

            return;
        }

        const Message* message = tcpSocket->receive();

        if (!message)
        {
            handleError("Failed to receive file.");

            return;
        }

        if (message->data->getProperty("ip")->asString().value_or("") != ip)
        {
            handleError("Invalid connection.");

            return;
        }

        std::cout << "received file: " << message->data->getProperty("name")->asString().value() << "\n";

        if (!tcpSocket->destroy())
        {
            handleError("Failed to destroy socket.");
        }
    });
}

#ifdef _WIN32

bool WinUDPSocket::create()
{
    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    if (socketHandle == INVALID_SOCKET)
    {
        return false;
    }

    bool val = true;

    return setsockopt(socketHandle, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof(bool)) != SOCKET_ERROR;
}

bool WinUDPSocket::socketBind(const std::string address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    return bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
}

bool WinUDPSocket::socketSend(const Message* message, const std::string address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    std::stringstream stream;

    message->serialize(stream);

    const std::string& str = stream.str();

    return sendto(socketHandle, str.c_str(), str.size() + 1, 0, (sockaddr*)&addr, sizeof(addr)) == str.size() + 1;
}

Message* WinUDPSocket::receive() const
{
    char buffer[BUFFER_SIZE];

    if (recv(socketHandle, buffer, BUFFER_SIZE, 0) == -1)
    {
        return nullptr;
    }

    buffer[BUFFER_SIZE - 1] = '\0';

    std::stringstream stream(buffer);

    return Message::deserialize(stream);
}

bool WinUDPSocket::destroy()
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

bool WinTCPSocket::create()
{
    socketHandle = socket(PF_INET, SOCK_STREAM, 0);

    return socketHandle != INVALID_SOCKET;
}

bool WinTCPSocket::socketBind(const std::string address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    return bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
}

bool WinTCPSocket::socketConnect(const std::string address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    return connect(socketHandle, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
}

bool WinTCPSocket::socketListen() const
{
    return listen(socketHandle, 1) != SOCKET_ERROR;
}

bool WinTCPSocket::socketAccept()
{
    SOCKET clientHandle = accept(socketHandle, nullptr, nullptr);

    if (clientHandle == INVALID_SOCKET)
    {
        return false;
    }

    if (closesocket(socketHandle) == SOCKET_ERROR)
    {
        return false;
    }

    socketHandle = clientHandle;

    return true;
}

bool WinTCPSocket::socketSend(const Message* message) const
{
    std::stringstream stream;

    message->serialize(stream);

    const std::string& str = stream.str();

    return send(socketHandle, str.c_str(), str.size() + 1, 0) == str.size() + 1;
}

Message* WinTCPSocket::receive() const
{
    std::stringstream stream;

    char buffer[BUFFER_SIZE];

    buffer[BUFFER_SIZE - 1] = '\0';

    int n;

    do
    {
        n = recv(socketHandle, buffer, BUFFER_SIZE - 1, 0);

        if (n == -1)
        {
            return nullptr;
        }

        stream << buffer;
    } while (n > 0);

    return Message::deserialize(stream);
}

bool WinTCPSocket::destroy()
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

WinNetworkManager::WinNetworkManager()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "Failed to initialize WinSock.\n";
    }
}

WinNetworkManager::~WinNetworkManager()
{
    WSACleanup();
}

UDPSocket* WinNetworkManager::newUDPSocket() const
{
    return new WinUDPSocket();
}

TCPSocket* WinNetworkManager::newTCPSocket() const
{
    return new WinTCPSocket();
}

std::string WinNetworkManager::getAddress() const
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

#else

bool BSDUDPSocket::create()
{
    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    if (socketHandle == -1)
    {
        return false;
    }

    int val = 1;

    return setsockopt(socketHandle, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) == 0;
}

bool BSDUDPSocket::socketBind(const std::string address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    return bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) == 0;
}

bool BSDUDPSocket::socketSend(const Message* message, const std::string address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    std::stringstream stream;

    message->serialize(stream);

    const std::string& str = stream.str();

    return sendto(socketHandle, str.c_str(), str.size() + 1, 0, (sockaddr*)&addr, sizeof(addr)) == str.size() + 1;
}

Message* BSDUDPSocket::receive() const
{
    char buffer[BUFFER_SIZE];

    if (recv(socketHandle, buffer, BUFFER_SIZE, 0) == -1)
    {
        return nullptr;
    }

    buffer[BUFFER_SIZE - 1] = '\0';

    std::stringstream stream(buffer);

    return Message::deserialize(stream);
}

bool BSDUDPSocket::destroy()
{
    if (shutdown(socketHandle, SHUT_RDWR) != 0)
    {
        return false;
    }

    socketHandle = -1;

    return true;
}

bool BSDTCPSocket::create()
{
    socketHandle = socket(PF_INET, SOCK_STREAM, 0);

    return socketHandle != -1;
}

bool BSDTCPSocket::socketBind(const std::string address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    return bind(socketHandle, (sockaddr*)&addr, sizeof(addr)) == 0;
}

bool BSDTCPSocket::socketConnect(const std::string address, const unsigned int port) const
{
    sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    return connect(socketHandle, (sockaddr*)&addr, sizeof(addr)) == 0;
}

bool BSDTCPSocket::socketListen() const
{
    return listen(socketHandle, 1) == 0;
}

bool BSDTCPSocket::socketAccept()
{
    int clientHandle = accept(socketHandle, nullptr, nullptr);

    if (clientHandle == -1)
    {
        return false;
    }

    socketHandle = clientHandle;

    return true;
}

bool BSDTCPSocket::socketSend(const Message* message) const
{
    std::stringstream stream;

    message->serialize(stream);

    const std::string& str = stream.str();

    return send(socketHandle, str.c_str(), str.size() + 1, 0) == str.size() + 1;
}

Message* BSDTCPSocket::receive() const
{
    std::stringstream stream;

    char buffer[BUFFER_SIZE];

    buffer[BUFFER_SIZE - 1] = '\0';

    int n;

    do
    {
        n = recv(socketHandle, buffer, BUFFER_SIZE - 1, 0);

        if (n == -1)
        {
            return nullptr;
        }

        stream << buffer;
    } while (n > 0);

    return Message::deserialize(stream);
}

bool BSDTCPSocket::destroy()
{
    if (shutdown(socketHandle, SHUT_RDWR) != 0)
    {
        return false;
    }

    socketHandle = -1;

    return true;
}

UDPSocket* BSDNetworkManager::newUDPSocket() const
{
    return new BSDUDPSocket();
}

TCPSocket* BSDNetworkManager::newTCPSocket() const
{
    return new BSDTCPSocket();
}

std::string BSDNetworkManager::getAddress() const
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

#endif
