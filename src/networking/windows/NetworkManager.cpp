#pragma comment(lib, "Ws2_32.lib")

#include <exception>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "../../utils/Logger.h"
#include "../NetworkManager.h"

NetworkManager::NetworkManager(int port, std::string ip_address)
{

    m_ip_address = ip_address;
    m_port = port;
    m_server_socket = NULL;
    m_server_address.sin_family = AF_INET;
    m_server_address.sin_port = htons(m_port);
    m_server_address.sin_addr.s_addr = m_ip_address == ANY_IP ? INADDR_ANY : inet_addr(m_ip_address.c_str());
}

std::string NetworkManager::getIp() const
{
    return m_ip_address;
}

int NetworkManager::getPort() const
{
    return m_port;
}

void NetworkManager::setupSocket()
{
    Logger &logger = Logger::getInstance();
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, "WSAStartup failed!");

        throw std::runtime_error("WSAStartup failed!");
    }

    m_server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (m_server_socket == INVALID_SOCKET)
    {
        throw std::runtime_error("Error when creating socket! (ip: " + m_ip_address +
                                 ", port: " + std::to_string(m_port));
    }

    int bindResult = bind(m_server_socket, (struct sockaddr *)&m_server_address, sizeof(m_server_address));
    if (bindResult < 0)
    {
        throw std::runtime_error("Binding socket faile! (ip: " + m_ip_address + ", port: " + std::to_string(m_port));

        closeSocket();
    }
}

void NetworkManager::closeSocket()
{
    closesocket(m_server_socket);
    WSACleanup();
}

void NetworkManager::startListening(const std::function<void(char *, int)> &callback, bool async)
{
    Logger &logger = Logger::getInstance();
    int addrlen = sizeof(m_server_address);

    if (listen(m_server_socket, MAX_CONNECTION_QUEUE) == SOCKET_ERROR)
    {
        throw std::runtime_error("Error trying to listen the socket! (ip: " + m_ip_address +
                                 ", port: " + std::to_string(m_port));
    }

    HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    if (iocp == NULL)
    {
        throw std::runtime_error("Failed to create IOCP");
    }

    Logger &logger = Logger::getInstance();

    int addrlen = sizeof(m_server_address);
    SOCKET client_socket;
    char buffer[MAX_BUFFER_LENGHT_FOR_REQUESTS] = {0};

    m_listening = true;

    // First loop will accept new connections, and associate them to the IOCP so it reads/sends data async
    while (m_listening)
    {

        client_socket = accept(m_server_socket, (struct sockaddr *)&m_server_address, &addrlen);

        if (client_socket == SOCKET_ERROR)
        {
            closeSocket();
            m_listening = false;
            throw std::runtime_error("Accept failed: " + WSAGetLastError());
        }

        Connection *conn = new Connection();
        std::pair<std::string, int> remoteAddressInfo = getRemoteAddress(client_socket);

        conn->ipAddress = remoteAddressInfo.first;
        conn->port = remoteAddressInfo.second;
        conn->sock = client_socket;

        ZeroMemory(&conn->recv_overlapped, sizeof(conn->recv_overlapped));

        WSABUF buff;
        buff.buf = conn->recv_buffer;
        buff.len = sizeof(conn->recv_buffer);
        DWORD flags = 0, bytes = 0;

        CreateIoCompletionPort((HANDLE)client_socket, iocp, (ULONG_PTR)conn, 0);

        int r = WSARecv(client_socket, &buff, 1, &flags, &bytes, &conn->recv_overlapped, NULL);
        if (r == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        {
            logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, "WSARecv failed");
            closesocket(client_socket);
            delete conn;
        }
    }

    // The following thread loop will be called when some socket receives data
    m_listener_thread = std::thread([iocp, &logger, &callback]() {
        DWORD bytes;
        ULONG_PTR completionKey;
        OVERLAPPED *overlapped;

        while (true)
        {
            BOOL ok = GetQueuedCompletionStatus(iocp, &bytes, &completionKey, &overlapped, INFINITE);
            Connection *conn = (Connection *)completionKey;

            if (!ok || bytes == 0) // Disconnected
            {
                // Disconnected
                logger.log(LogType::NETWORK, LogSeverity::LOG_INFO,
                           "Client disconnected: " + std::to_string(conn->sock));
                closesocket(conn->sock);
                delete conn;
                continue;
            }

            callback(conn, conn->recv_buffer);
        }
    });

    m_listener_thread.detach();
}

std::pair<std::string, int> NetworkManager::getRemoteAddress(SOCKET &remoteSocket)
{

    Logger &logger = Logger::getInstance();
    sockaddr_in remoteAddress;
    int remote_addr_len = sizeof(remoteAddress);
    SOCKET result = getpeername(remoteSocket, (struct sockaddr *)&remoteAddress, &remote_addr_len);

    if (result == SOCKET_ERROR)
    {
        closesocket(remoteSocket);
        closesocket(m_server_socket);
        WSACleanup();
        throw std::runtime_error("Error getting remote address");
    }

    char ip_address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &remoteAddress.sin_addr, ip_address, sizeof(ip_address));
    int port = ntohs(remoteAddress.sin_port);

    return {ip_address, port};
}

void NetworkManager::startListenerLoop(const std::function<void(char *, int)> &callback)
{

    Logger &logger = Logger::getInstance();

    int addrlen = sizeof(m_server_address);
    SOCKET client_socket;
    char buffer[MAX_BUFFER_LENGHT_FOR_REQUESTS] = {0};

    while (m_listening)
    {

        client_socket = accept(m_server_socket, (struct sockaddr *)&m_server_address, &addrlen);

        if (client_socket == SOCKET_ERROR)
        {
            closeSocket();
            m_listening = false;
            throw std::runtime_error("Accept failed: " + WSAGetLastError());
        }

        int bytes_received = recv(client_socket, buffer, MAX_BUFFER_LENGHT_FOR_REQUESTS, 0);
        std::pair<std::string, int> remoteAddressInfo = getRemoteAddress(client_socket);

        std::string remote_ip_address = remoteAddressInfo.first;
        int remote_port = remoteAddressInfo.second;

        logger.log(LogType::NETWORK, LogSeverity::LOG_INFO,
                   "Recieved connection from " + remote_ip_address + ":" + std::to_string(remote_port));

        callback(buffer, bytes_received);
    }
}
