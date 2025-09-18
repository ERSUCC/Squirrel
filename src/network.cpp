#include "../include/network.h"

NetworkManager::NetworkManager(ErrorHandler* errorHandler, const std::string name, const std::string address) :
    errorHandler(errorHandler), name(name), address(address)
{
    if (name.empty())
    {
        errorHandler->push(SquirrelException("Failed to get computer name."));
    }

    if (address.empty())
    {
        errorHandler->push(SquirrelSocketException("Failed to find a valid network interface."));
    }
}

void NetworkManager::beginService(const std::function<void(const std::string, const std::string)> handleResponse, const std::function<void(const std::string)> handleConnect)
{
    udpSocket = newUDPSocket();

    if (!udpSocket->create(address))
    {
        errorHandler->push(SquirrelSocketException("Failed to create socket."));

        return;
    }

    if (!udpSocket->socketBind("0.0.0.0", UDP_PORT))
    {
        errorHandler->push(SquirrelSocketException("Failed to bind socket."));

        return;
    }

    broadcastThread = std::thread([=]()
    {
        std::chrono::high_resolution_clock clock;
        std::chrono::time_point<std::chrono::high_resolution_clock> last;

        while (udpSocket->isAlive())
        {
            if ((clock.now() - last).count() >= 1e9)
            {
                const Message* broadcastMessage = new Message(new JSONObject(
                {
                    { "type", new JSONString("broadcast") },
                    { "name", new JSONString(name) },
                    { "ip", new JSONString(address) }
                }));

                if (!udpSocket->socketSend(broadcastMessage, "255.255.255.255", UDP_PORT))
                {
                    errorHandler->push(SquirrelSocketException("Failed to broadcast message."));

                    return;
                }

                last = clock.now();
            }
        }
    });

    responseThread = std::thread([=]()
    {
        while (const Message* message = udpSocket->receive())
        {
            const std::optional<std::string> type = message->data->getProperty("type")->asString();

            if (type == "available")
            {
                const std::optional<std::string> name = message->data->getProperty("name")->asString();
                const std::optional<std::string> ip = message->data->getProperty("ip")->asString();

                if (name && ip && ip != address)
                {
                    handleResponse(name.value(), ip.value());
                }
            }

            else if (type == "broadcast")
            {
                const std::optional<std::string> ip = message->data->getProperty("ip")->asString();

                if (ip && ip != address)
                {
                    const Message* response = new Message(new JSONObject(
                    {
                        { "type", new JSONString("available") },
                        { "name", new JSONString(name) },
                        { "ip", new JSONString(address) }
                    }));

                    if (!udpSocket->socketSend(response, ip.value(), UDP_PORT))
                    {
                        errorHandler->push(SquirrelSocketException("Failed to respond to broadcast."));

                        return;
                    }
                }
            }

            else if (type == "connect")
            {
                if (const std::optional<std::string> ip = message->data->getProperty("ip")->asString())
                {
                    handleConnect(ip.value());
                }
            }
        }
    });
}

void NetworkManager::beginConnect(const std::string ip)
{
    const Message* connectMessage = new Message(new JSONObject(
    {
        { "type", new JSONString("connect") },
        { "ip", new JSONString(address) }
    }));

    if (!udpSocket->socketSend(connectMessage, ip, UDP_PORT))
    {
        errorHandler->push(SquirrelSocketException("Failed to send connection request."));

        return;
    }
}

void NetworkManager::beginTransfer(const std::filesystem::path path, const std::string ip)
{
    std::ifstream file(path, std::ios_base::binary);

    if (!file.is_open())
    {
        errorHandler->push(SquirrelFileException("Failed to open specified file."));

        return;
    }

    std::stringstream stream;

    stream << file.rdbuf();

    file.close();

    const std::string fileName = path.filename().string();
    const std::string data = Base64::encode(stream.str());

    if (transferThread.joinable())
    {
        transferThread.join();
    }

    const Message* message = new Message(new JSONObject(
    {
        { "type", new JSONString("transfer") },
        { "name", new JSONString(name) },
        { "ip", new JSONString(address) },
        { "file", new JSONString(fileName) },
        { "data", new JSONString(data) }
    }));

    tcpSocket = newTCPSocket();

    if (!tcpSocket->create())
    {
        errorHandler->push(SquirrelSocketException("Failed to create socket."));

        return;
    }

    transferThread = std::thread([=]()
    {
        if (!tcpSocket->socketConnect(ip, TCP_PORT))
        {
            errorHandler->push(SquirrelSocketException("Failed to connect to socket."));

            return;
        }

        if (!tcpSocket->socketSend(message))
        {
            errorHandler->push(SquirrelSocketException("Failed to transfer file."));

            return;
        }

        if (!tcpSocket->destroy())
        {
            errorHandler->push(SquirrelSocketException("Failed to destroy socket."));
        }
    });
}

void NetworkManager::beginReceive(const std::string ip, const std::function<void(const std::string, const std::string&)> handleReceive)
{
    if (transferThread.joinable())
    {
        transferThread.join();
    }

    tcpSocket = newTCPSocket();

    if (!tcpSocket->create())
    {
        errorHandler->push(SquirrelSocketException("Failed to create socket."));

        return;
    }

    if (!tcpSocket->socketBind(address, TCP_PORT))
    {
        errorHandler->push(SquirrelSocketException("Failed to bind socket."));

        return;
    }

    if (!tcpSocket->socketListen())
    {
        errorHandler->push(SquirrelSocketException("Failed to listen on socket."));

        return;
    }

    transferThread = std::thread([=]()
    {
        if (!tcpSocket->socketAccept())
        {
            errorHandler->push(SquirrelSocketException("Failed to accept connection."));

            return;
        }

        const Message* message = tcpSocket->receive();

        if (!message)
        {
            errorHandler->push(SquirrelSocketException("Failed to receive file."));

            return;
        }

        if (message->data->getProperty("ip")->asString() != ip)
        {
            errorHandler->push(SquirrelSocketException("Invalid connection."));

            return;
        }

        const std::optional<std::string> fileName = message->data->getProperty("file")->asString();
        const std::optional<std::string> data = message->data->getProperty("data")->asString();

        if (!fileName || !data)
        {
            errorHandler->push(SquirrelSocketException("Received incorrect message format."));

            return;
        }

        handleReceive(fileName.value(), Base64::decode(data.value()));

        if (!tcpSocket->destroy())
        {
            errorHandler->push(SquirrelSocketException("Failed to destroy socket."));

            return;
        }
    });
}

#ifdef _WIN32

bool WinUDPSocket::create(const std::string address)
{
    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    if (socketHandle == INVALID_SOCKET)
    {
        return false;
    }

    bool val = true;

    if (setsockopt(socketHandle, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof(bool)) == SOCKET_ERROR)
    {
        return false;
    }

    unsigned long addr = inet_addr(address.c_str());

    return setsockopt(socketHandle, IPPROTO_IP, IP_UNICAST_IF, (char*)&addr, sizeof(addr)) != SOCKET_ERROR;
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

bool WinUDPSocket::isAlive()
{
    return socketHandle != INVALID_SOCKET;
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

    int n;

    do
    {
        n = recv(socketHandle, buffer, BUFFER_SIZE - 1, 0);

        if (n == -1)
        {
            return nullptr;
        }

        buffer[n] = '\0';

        stream << buffer;
    } while (n > 0);

    return Message::deserialize(stream);
}

bool WinTCPSocket::destroy()
{
    if (shutdown(socketHandle, SD_BOTH) == SOCKET_ERROR && WSAGetLastError() != WSAENOTCONN)
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

WinNetworkManager::WinNetworkManager(ErrorHandler* errorHandler) :
    NetworkManager(errorHandler, getName(), getAddress())
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        errorHandler->push(SquirrelSocketException("Failed to initialize WinSock."));
    }
}

WinNetworkManager::~WinNetworkManager()
{
    WSACleanup();
}

unsigned int WinNetworkManager::convertAddress(const std::string address) const
{
    return inet_addr(address.c_str());
}

std::string WinNetworkManager::convertAddress(const unsigned int address) const
{
    in_addr addr;

    addr.s_addr = address;

    return inet_ntoa(addr);
}

UDPSocket* WinNetworkManager::newUDPSocket() const
{
    return new WinUDPSocket();
}

TCPSocket* WinNetworkManager::newTCPSocket() const
{
    return new WinTCPSocket();
}

std::string WinNetworkManager::getName() const
{
    char buffer[UNLEN + 1];

    unsigned long size = UNLEN + 1;

    if (!GetUserName(buffer, &size))
    {
        return "";
    }

    return buffer;
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
        NL_NETWORK_CONNECTIVITY_HINT hint;

        if (GetNetworkConnectivityHintForInterface(addrs->IfIndex, &hint) == NO_ERROR && hint.ConnectivityLevel == NetworkConnectivityLevelHintInternetAccess)
        {
            if (addrs->IfType != IF_TYPE_SOFTWARE_LOOPBACK && addrs->OperStatus == IfOperStatusUp)
            {
                break;
            }
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

bool BSDUDPSocket::create(const std::string address)
{
    socketHandle = socket(PF_INET, SOCK_DGRAM, 0);

    if (socketHandle == -1)
    {
        return false;
    }

    int val = 1;

    if (setsockopt(socketHandle, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) == -1)
    {
        return false;
    }

    ip_mreq req;

    req.imr_interface.s_addr = inet_addr(address.c_str());
    req.imr_multiaddr.s_addr = inet_addr(address.c_str());

    return setsockopt(socketHandle, IPPROTO_IP, IP_MULTICAST_IF, &req, sizeof(req)) != -1;
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

bool BSDUDPSocket::isAlive()
{
    return socketHandle != -1;
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

    int n;

    do
    {
        n = recv(socketHandle, buffer, BUFFER_SIZE - 1, 0);

        if (n == -1)
        {
            return nullptr;
        }

        buffer[n] = '\0';

        stream << buffer;
    } while (n > 0);

    return Message::deserialize(stream);
}

bool BSDTCPSocket::destroy()
{
    if (shutdown(socketHandle, SHUT_RDWR) != 0 && errno != ENOTCONN)
    {
        return false;
    }

    socketHandle = -1;

    return true;
}

BSDNetworkManager::BSDNetworkManager(ErrorHandler* errorHandler) :
    NetworkManager(errorHandler, getName(), getAddress()) {}

unsigned int BSDNetworkManager::convertAddress(const std::string ip) const
{
    return inet_addr(ip.c_str());
}

std::string BSDNetworkManager::convertAddress(const unsigned int address) const
{
    in_addr addr;

    addr.s_addr = address;

    return inet_ntoa(addr);
}

UDPSocket* BSDNetworkManager::newUDPSocket() const
{
    return new BSDUDPSocket();
}

TCPSocket* BSDNetworkManager::newTCPSocket() const
{
    return new BSDTCPSocket();
}

std::string BSDNetworkManager::getName() const
{
    const char* name = std::getenv("USER");

    if (name)
    {
        return name;
    }

    return "";
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
