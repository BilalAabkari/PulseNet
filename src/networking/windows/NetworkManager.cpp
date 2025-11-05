#include <exception>
#include <iostream>

#include "../../utils/Logger.h"
#include "../NetworkManager.h"

// clang-format on
namespace pulse::net
{

NetworkManager::NetworkManager(int port, std::string ip_address, int assembler_workers,
                               std::unique_ptr<TCPMessageAssembler> assembler)
    : m_listening(false), m_assembler_thread_pool(assembler_workers, [this]() { this->assemblerWorker(); }),
      m_ip_address(ip_address), m_port(port)
{

    m_server_address.sin_family = AF_INET;
    m_server_address.sin_port = htons(m_port);

    if (m_ip_address == ANY_IP)
    {
        m_server_address.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        InetPton(AF_INET, m_ip_address.c_str(), &m_server_address.sin_addr);
    }

    if (!assembler)
    {
        m_assembler = std::make_unique<DefaultMessageAssembler>();
    }
    else
    {
        m_assembler = std::move(assembler);
    }
}

void NetworkManager::stop()
{
}

std::string NetworkManager::getIp() const
{
    return m_ip_address;
}

int NetworkManager::getPort() const
{
    return m_port;
}

Client *NetworkManager::addClient(uint64_t id, int port, std::string ipAddress, SOCKET sock)
{
    std::unique_lock lock(m_mtx);

    std::unique_ptr client = std::make_unique<Client>(id, port, ipAddress, sock);

    m_client_list.insert({client->getId(), std::move(client)});

    m_client_list.at(id)->increaseReferenceCount();
    return m_client_list.at(id).get();
}

Client *NetworkManager::getClient(uint64_t id)
{
    std::shared_lock lock(m_mtx);

    auto it = m_client_list.find(id);

    if (it != m_client_list.end() && !it->second->isDisconnecting())
    {
        it->second->increaseReferenceCount();
        return it->second.get();
    }
    else
    {
        return nullptr;
    }
}

std::unique_ptr<NetworkManager::Request> NetworkManager::next()
{
    return m_requests_queue.pop();
}

void NetworkManager::terminateClient(uint64_t id)
{
    std::lock_guard lock(m_mtx);
    auto i = m_client_list.find(id);

    if (i != m_client_list.end() && i->second->isDisconnecting())
    {
        closesocket(i->second->getSocket());
        m_client_list.erase(i);
    }
}

void NetworkManager::setupSocket()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        throw std::runtime_error("WSAStartup failed!");
    }

    m_server_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

    if (m_server_socket == INVALID_SOCKET)
    {
        WSACleanup();
        throw std::runtime_error("Error when creating socket! (ip: " + m_ip_address +
                                 ", port: " + std::to_string(m_port));
    }

    int bindResult = bind(m_server_socket, (struct sockaddr *)&m_server_address, sizeof(m_server_address));
    if (bindResult == SOCKET_ERROR)
    {
        closeSocket();
        throw std::runtime_error("Binding socket failed! (ip: " + m_ip_address + ", port: " + std::to_string(m_port) +
                                 ")");
    }
}

void NetworkManager::closeSocket()
{
    closesocket(m_server_socket);
    WSACleanup();
}

void NetworkManager::showClients(std::ostream &os) const
{
    std::shared_lock lock(m_mtx);

    os << std::left << std::setfill(' ') << std::setw(10) << "Client Id" << std::setw(3) << "|" << std::setw(24)
       << " Ip Address" << std::setw(3) << "|" << std::setw(5) << "Port" << std::setw(3) << "|" << std::setw(5)
       << "Refs" << std::setw(3) << "|" << std::setw(14) << "Disconnecting\n";
    os << "---------------------------------------------------------------------\n";

    for (const auto &item : m_client_list)
    {
        item.second->showInfo(std::cout);
    }
}

void NetworkManager::startListening()
{
    setupSocket();

    if (m_listening)
    {
        return;
    }

    Logger &logger = Logger::getInstance();

    GUID guid = WSAID_ACCEPTEX;
    DWORD bytes;
    int res = WSAIoctl(m_server_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &acceptEx,
                       sizeof(acceptEx), &bytes, NULL, NULL);

    if (res == SOCKET_ERROR)
    {
        throw std::runtime_error("Error getting acceptEx ptr");
    }

    if (listen(m_server_socket, MAX_CONNECTION_QUEUE) == SOCKET_ERROR)
    {
        throw std::runtime_error("Error trying to listen the socket! (ip: " + m_ip_address +
                                 ", port: " + std::to_string(m_port));
    }

    iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    if (iocp == NULL)
    {
        throw std::runtime_error("Failed to create IOCP");
    }

    HANDLE result = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_server_socket), iocp, 0, 0);

    if (result == NULL)
    {
        throw std::runtime_error("Failed to associate server socket with IOCP");
    }

    m_accept_ctx = new AcceptContext();
    bool success = postAcceptExEvent(*m_accept_ctx);
    m_listening = true;
    m_pending_accepts.fetch_add(1);

    if (!success)
    {
        int err = WSAGetLastError();

        if (err != ERROR_IO_PENDING)
        {
            closeSocket();
            throw new std::runtime_error("Error executing first acceptEX" + std::to_string(WSAGetLastError()));
        }
    }

    m_listener_thread = std::thread([this]() {
        DWORD bytes;
        ULONG_PTR completionKey;
        OVERLAPPED *overlapped;

        Logger &logger = Logger::getInstance();

        while (m_listening)
        {
            BOOL ok = GetQueuedCompletionStatus(iocp, &bytes, &completionKey, &overlapped, INFINITE);

            // New connection case
            if (&m_accept_ctx->overlapped == overlapped)
            {
                m_pending_accepts.fetch_sub(1);

                setsockopt(m_accept_ctx->client_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&m_server_socket,
                           sizeof(m_server_socket));

                std::pair<int, std::string> address = getRemoteAddressFromAcceptContext(*m_accept_ctx);

                Client *client =
                    addClient(AUTOGENERATED_ID.load(), address.first, address.second, m_accept_ctx->client_socket);

                AUTOGENERATED_ID.fetch_add(1);

                CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_accept_ctx->client_socket), iocp,
                                       reinterpret_cast<ULONG_PTR>(client), 0);

                client->increaseReferenceCount();
                postReceiveEvent(*client);
                client->decreaseReferenceCount();

                client->decreaseReferenceCount();

                if (client->isDisconnecting() && client->getReferenceCount() == 0)
                {
                    terminateClient(client->getId());
                }

                delete m_accept_ctx;
                m_accept_ctx = new AcceptContext();

                // Keep listening
                bool success = postAcceptExEvent(*m_accept_ctx);
                if (success)
                {
                    m_pending_accepts.fetch_add(1);
                }

                // And push an event to read from the just connected client
            }
            else // Already existent connection
            {

                Client *client = reinterpret_cast<Client *>(completionKey);
                client->decreaseReferenceCount();

                if (client->isDisconnecting() && client->getReferenceCount() == 0)
                {
                    terminateClient(client->getId());
                }
                else
                {

                    if (overlapped == client->getReadOverlapped()) // Read case
                    {

                        if (bytes == 0) // Client disconnected
                        {
                            client->disconnect();

                            if (client->getReferenceCount() == 0)
                            {
                                terminateClient(client->getId());
                            }
                        }
                        else
                        {

                            client->m_recv_len = bytes;
                            m_assembling_queue.push(std::make_unique<uint64_t>(client->getId()));
                        }
                    }
                    else if (overlapped == client->getSendOverlapped())
                    {
                        std::lock_guard lock(client->m_send_mtx);

                        if (!client->m_outbound_message_queue.empty())
                        {
                            std::string message = std::move(client->m_outbound_message_queue.front());
                            client->m_outbound_message_queue.pop();

                            client->increaseReferenceCount();
                            postSendEvent(*client, message);
                            client->decreaseReferenceCount();

                            if (client->isDisconnecting() && client->getReferenceCount() == 0)
                            {
                                terminateClient(client->getId());
                            }
                        }
                        else
                        {
                            client->m_is_sending = false;
                        }
                    }
                }
            }
        }
    });

    m_assembler_thread_pool.run();
}

void NetworkManager::send(uint64_t id, const std::string &message)
{
    Logger &logger = Logger::getInstance();

    Client *client = getClient(id);

    if (client)
    {
        std::lock_guard lock(client->m_send_mtx);

        if (!client->m_is_sending)
        {
            client->increaseReferenceCount();
            postSendEvent(*client, message);
            client->decreaseReferenceCount();
        }
        else
        {
            client->m_outbound_message_queue.push(message);
        }

        client->decreaseReferenceCount();

        if (client->isDisconnecting() && client->getReferenceCount() == 0)
        {
            terminateClient(client->getId());
        }
    }
    else
    {
        logger.log(LogType::NETWORK, LogSeverity::LOG_WARNING,
                   "Tried to send a message to client " + std::to_string(id) + " but it's not connected");
    }
}

std::unique_ptr<NetworkManager::Request> NetworkManager::createRequest(Client &client)
{

    std::unique_ptr<Request> request = std::make_unique<Request>();
    request->client.id = client.getId();

    std::pair<int, std::string> address = client.getAddress();
    request->client.ip_address = std::move(address.second);
    request->client.port = address.first;

    return request;
}

bool NetworkManager::postAcceptExEvent(AcceptContext &accept_context)
{
    Logger &logger = Logger::getInstance();

    if (m_server_socket == NULL)
    {
        logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR,
                   "Tried to post an accept event to IOCP, but the server socket "
                   "is not set");
        return false;
    }

    if (m_accept_ctx == nullptr)
    {
        logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR,
                   "Tried to post an accept event to IOCP, but the accept context object "
                   "is not created");
        return false;
    }

    DWORD bytes_accept = 0;
    SOCKET client_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

    accept_context.client_socket = client_socket;

    ZeroMemory(&accept_context.overlapped, sizeof(accept_context.overlapped));

    BOOL acceptEx_result =
        acceptEx(m_server_socket, client_socket, accept_context.rcv_buffer, 0, sizeof(m_server_address) + 16,
                 sizeof(m_server_address) + 16, &bytes_accept, &accept_context.overlapped);

    if (!acceptEx_result && WSAGetLastError() != WSA_IO_PENDING)
    {
        int error_code = WSAGetLastError();
        logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, "AcceptEx failed with error code: " + error_code);

        closesocket(accept_context.client_socket);
        accept_context.client_socket = INVALID_SOCKET;
        return false;
    }
    else
    {
        return true;
    }
}

void NetworkManager::postReceiveEvent(Client &client)
{
    Logger &logger = Logger::getInstance();

    bool error = false;

    DWORD flags = 0;
    DWORD bytes_received = 0;
    WSABUF wsa_buf;
    wsa_buf.buf = client.m_recv_buffer + client.m_recv_len;
    wsa_buf.len = MAX_BUFFER_LENGHT_FOR_REQUESTS - client.m_recv_len;

    OVERLAPPED *read_overlapped = client.getReadOverlapped();
    ZeroMemory(read_overlapped, sizeof(*read_overlapped));

    // TODO: This can end immediately if it already has data, we need to manage pushing to queue and recursive call.
    int result = WSARecv(client.getSocket(), &wsa_buf, 1, &bytes_received, &flags, read_overlapped, NULL);

    if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
    {
        int error_code = WSAGetLastError();
        logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, "WSARecv failed with error code: " + error_code);
        error = true;
    }
    else if (result == 0) // Completed immediately
    {
        client.m_recv_len = bytes_received;
        m_assembling_queue.push(std::make_unique<uint64_t>(client.getId()));
    }
    else
    {
        client.increaseReferenceCount();
    }

    if (error)
    {
        client.disconnect();
    }
}

void NetworkManager::postSendEvent(Client &client, std::string message)
{
    Logger &logger = Logger::getInstance();

    if (message.size() > MAX_BUFFER_LENGHT_FOR_REQUESTS)
    {
        logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, "Message is larger than max lenght.");
        client.disconnect();
    }
    else
    {
        strcpy_s(client.m_send_buffer, MAX_BUFFER_LENGHT_FOR_REQUESTS, message.c_str());
        client.m_send_len = message.size();

        DWORD bytesSent = 0;
        DWORD flags = 0;

        WSABUF wsa_buf;
        wsa_buf.buf = client.m_send_buffer;
        wsa_buf.len = static_cast<ULONG>(client.m_send_len);

        OVERLAPPED *send_overlapped = client.getSendOverlapped();
        ZeroMemory(send_overlapped, sizeof(*send_overlapped));

        int result = WSASend(client.getSocket(), &wsa_buf, 1, &bytesSent, flags, send_overlapped, NULL);

        if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        {
            int error_code = WSAGetLastError();
            logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, "WSARecv failed with error code: " + error_code);
            client.disconnect();
        }
        else if (result == 0)
        {

            bool success = PostQueuedCompletionStatus(iocp, bytesSent, reinterpret_cast<ULONG_PTR>(&client),
                                                      client.getSendOverlapped());
            if (!success)
            {
                int error_code = WSAGetLastError();
                logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR,
                           "PostQueuedCompletionStatus failed with error code: " + error_code);

                client.disconnect();
            }
            else
            {
                client.m_is_sending = true;
                client.increaseReferenceCount();
            }
        }
        else
        {
            client.m_is_sending = true;
            client.increaseReferenceCount();
        }
    }
}

std::pair<int, std::string> NetworkManager::getRemoteAddressFromAcceptContext(AcceptContext &accept_context)
{

    sockaddr_in remote_in{};

    int remote_size = sizeof(remote_in);

    int r = getpeername(accept_context.client_socket, reinterpret_cast<sockaddr *>(&remote_in), &remote_size);

    char client_ip[INET_ADDRSTRLEN];
    DWORD client_port;

    inet_ntop(AF_INET, &(remote_in.sin_addr), client_ip, INET_ADDRSTRLEN);
    client_port = ntohs(remote_in.sin_port);

    return {client_port, client_ip};
}

void NetworkManager::assemblerWorker()
{
    while (m_listening)
    {
        std::unique_ptr<uint64_t> client_id = m_assembling_queue.pop();
        Client *client = getClient(*client_id);
        if (client)
        {

            TCPMessageAssembler::AssemblingResult result =
                m_assembler->feed(client->getId(), client->m_recv_buffer, client->m_recv_len,
                                  MAX_BUFFER_LENGHT_FOR_REQUESTS, client->getLastBytesReceived());

            if (result.error)
            {
                send(client->getId(), result.error_message);
                client->disconnect();
            }
            else
            {

                for (std::string message : result.messages)
                {
                    std::unique_ptr<Request> r = createRequest(*client);
                    r->message = std::move(message);
                    m_requests_queue.push(std::move(r));
                }

                // Then we post the next receive:
                client->increaseReferenceCount();
                postReceiveEvent(*client);
                client->decreaseReferenceCount();
            }
        }
    }
}

} // namespace pulse::net
