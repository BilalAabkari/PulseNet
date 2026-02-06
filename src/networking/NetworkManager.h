#pragma once

#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>

#include "../utils/Logger.h"
#include "Client.h"
#include "DefaultMessageAssembler.h"
#include "NetworkPlatform.h"
#include "TCPMessageAssembler.h"
#include "ThreadPool.h"
#include "ThreadSafeQueue.h"
#include "constants.h"

// clang-format off
#include <Wsrm.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <mswsock.h>

#pragma comment(lib, "Ws2_32.lib")

// clang-format on

namespace pulse::net
{

/**
 * @class NetworkManager
 * @brief A class that manages networking and socket listeners.
 *
 * This class manages the setup, control of network sockets and listeners.
 */

template <typename T>
concept ValidAssembler = requires {
    typename T::MessageType;
} && std::derived_from<T, TCPMessageAssembler<typename T::MessageType>> && std::movable<typename T::MessageType>;
;

template <ValidAssembler Assembler> class NetworkManager
{

  public:
    using MessageType = typename Assembler::MessageType;

    struct Request
    {
        ClientDto client;
        MessageType message;
    };

    /* ----------------
     * Constructors
     * ----------------
     */
    NetworkManager(int port, std::string ip_address = ANY_IP, int assembler_workers = 2,
                   std::unique_ptr<Assembler> assembler = nullptr)
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
            throw std::invalid_argument("Assembler cannot be null");
        }
        else
        {
            m_assembler = std::move(assembler);
        }
    }

    NetworkManager(const NetworkManager &nm) = delete;
    NetworkManager(const NetworkManager &&nm) = delete;
    NetworkManager &operator=(const NetworkManager &nm) = delete;
    NetworkManager &operator=(NetworkManager &&nm) = delete;

    /* ----------------
     * Public methods
     * ----------------
     */

    /**
     *  @brief Stops de listener safely.
     */
    void stop()
    {
    }

    /**
     * @brief Starts the socket listener
     */
    void startListening()
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

                    setsockopt(m_accept_ctx->client_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                               (char *)&m_server_socket, sizeof(m_server_socket));

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

    void send(uint64_t id, const std::string &message)
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

    void showClients(std::ostream &os) const
    {
        std::shared_lock lock(m_mtx);

        os << std::left << std::setfill(' ') << std::setw(10) << "Client Id" << std::setw(3) << "|" << std::setw(24)
           << " Ip Address" << std::setw(3) << "|" << std::setw(6) << "Port" << std::setw(3) << "|" << std::setw(5)
           << "Refs" << std::setw(3) << "|" << std::setw(14) << "Disconnecting\n";
        os << "--------------------------------------------------------------------"
              "-\n";

        for (const auto &item : m_client_list)
        {
            item.second->showInfo(std::cout);
        }
    }

    std::unique_ptr<Request> next()
    {
        return m_requests_queue.pop();
    }

    std::string getIp() const
    {
        return m_ip_address;
    }

    int getPort() const
    {
        return m_port;
    }

  private:
    /* ----------------
     * Private attributes
     * ----------------
     */

    /**
     * @struct AcceptContext
     * @brief Context structure for Windows overlapped I/O accept operations.
     *
     * This structure encapsulates the necessary data for asynchronous socket
     * acceptance on Windows platforms using I/O Completion Ports (IOCP).
     *
     * @note Windows-specific: Only compiled when _WIN32 is defined.
     */
    struct AcceptContext
    {
        OVERLAPPED overlapped;
        SOCKET client_socket;
        char rcv_buffer[MAX_BUFFER_LENGHT_FOR_REQUESTS];
    };

    /**
     * @brief Pointer to the accept context for Windows async operations.
     *
     * Maintains the state of pending accept operations when using overlapped
     * I/O. Must be allocated/deallocated appropriately to prevent memory leaks.
     *
     * @note Windows-specific: Only available when _WIN32 is defined.
     */
    AcceptContext *m_accept_ctx = nullptr;

    /**
     * @brief Atomic counter tracking the number of pending asynchronous accept
     * operations.
     *
     * Maintains a count of AcceptEx calls that have been initiated but not yet
     * completed. This is used to ensure proper cleanup during shutdown (wait for
     * pending operations):
     *
     * The counter is incremented when AcceptEx is called and decremented when:
     * - An accept operation completes successfully
     * - An accept operation fails
     * - An operation is cancelled during shutdown
     *
     * @note Atomic to ensure thread-safety when accessed from both the listener
     *       thread and I/O completion port worker threads.
     *
     * @note On Windows with IOCP, multiple AcceptEx operations may be pending
     *       simultaneously to improve connection acceptance throughput.
     */
    std::atomic<int> m_pending_accepts = 0;

    HANDLE iocp;

    LPFN_ACCEPTEX acceptEx = nullptr;

    /**
     * @brief IP address on which the server listens for incoming connections.
     *
     * Stores the network interface address (e.g., "0.0.0.0" for all interfaces,
     * "127.0.0.1" for localhost, or a specific IP address).
     */
    std::string m_ip_address;

    /**
     * @brief Socket address structure containing server binding information.
     *
     * Contains the address family (AF_INET), IP address, and port number
     * used to bind the server socket.
     */
    sockaddr_in m_server_address;

    /**
     * @brief Port number on which the server listens for connections.
     *
     * Valid range: 1-65535 (typically > 1024 for non-privileged applications).
     */
    int m_port;

    /**
     * @brief Background thread that handles incoming connection requests.
     *
     * Runs the accept loop, continuously listening for and processing
     * new client connections. Lifecycle managed by server start/stop
     * operations.
     */
    std::thread m_listener_thread;

    /**
     * @brief The main server socket descriptor/handle.
     *
     * Platform-specific socket type (SOCKET on Windows, int on Unix-like
     * systems). Used for binding to the address/port and accepting incoming
     * connections.
     */
    SOCKET_TYPE m_server_socket = NULL;

    /**
     * @brief Atomic counter for generating unique client identifiers.
     *
     */
    std::atomic<uint64_t> AUTOGENERATED_ID = 1;

    /**
     * @brief Atomic flag indicating whether the server is actively listening.
     *
     * Used to control the listener thread's execution and signal shutdown.
     * - true: Server is accepting connections
     * - false: Server should stop accepting new connections
     *
     * @note Atomic to allow safe checking across threads without locks.
     */
    std::atomic<bool> m_listening = false;

    /**
     * @brief Mutex for protecting concurrent access to the client list.
     *
     * Synchronizes operations that modify or iterate over m_client_list,
     * preventing race conditions when multiple threads access client data.
     */
    mutable std::shared_mutex m_mtx;

    /**
     * @brief Map of all currently connected clients, indexed by unique ID.
     *
     * Stores smart pointers to Client objects, where:
     * - Key: Unique client ID (generated from AUTOGENERATED_ID)
     * - Value: Unique pointer to Client instance (automatic cleanup on removal)
     *
     * Access must be synchronized using m_mtx to ensure thread-safety.
     *
     */
    std::unordered_map<uint64_t, std::unique_ptr<Client>> m_client_list;

    ThreadSafeQueue<Request> m_requests_queue{};

    ThreadSafeQueue<uint64_t> m_assembling_queue{};

    ThreadPool m_assembler_thread_pool;

    std::unique_ptr<Assembler> m_assembler;

    Client *addClient(uint64_t id, int port, std::string ipAddress, SOCKET sock)
    {
        std::unique_lock lock(m_mtx);

        std::unique_ptr client = std::make_unique<Client>(id, port, ipAddress, sock);

        m_client_list.insert({client->getId(), std::move(client)});

        m_client_list.at(id)->increaseReferenceCount();
        return m_client_list.at(id).get();
    }

    Client *getClient(uint64_t id)
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

    void terminateClient(uint64_t id)
    {
        std::lock_guard lock(m_mtx);
        auto i = m_client_list.find(id);

        if (i != m_client_list.end() && i->second->isDisconnecting())
        {
            closesocket(i->second->getSocket());
            m_client_list.erase(i);
        }
    }

    /*
     * @bried Creates a request after reveiving from client
     */
    std::unique_ptr<Request> createRequest(Client &client, Assembler::MessageType message)
    {
        std::unique_ptr<Request> request = std::make_unique<Request>();
        request->client.id = client.getId();

        std::pair<int, std::string> address = client.getAddress();
        request->client.ip_address = std::move(address.second);
        request->client.port = address.first;
        request->message = std::move(message);

        return request;
    }

    /* ----------------
     * Private methdos
     * ----------------
     */

    void assemblerWorker()
    {
        while (m_listening)
        {
            std::unique_ptr<uint64_t> client_id = m_assembling_queue.pop();
            Client *client = getClient(*client_id);
            if (client)
            {

                typename Assembler::AssemblingResult result =
                    m_assembler->feed(client->getId(), client->m_recv_buffer, client->m_recv_len,
                                      MAX_BUFFER_LENGHT_FOR_REQUESTS, client->getLastBytesReceived());

                if (result.error)
                {
                    send(client->getId(), result.error_message);
                    client->disconnect();
                }
                else
                {

                    for (typename Assembler::MessageType message : result.messages)
                    {
                        std::unique_ptr<Request> r = createRequest(*client, std::move(message));
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

    /**
     * @brief Creates the network socket
     */
    void setupSocket()
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
            throw std::runtime_error("Binding socket failed! (ip: " + m_ip_address +
                                     ", port: " + std::to_string(m_port) + ")");
        }
    }

    /**
     * @brief Posts an asynchronous AcceptEx operation to accept incoming client
     * connections.
     *
     * This function initiates an overlapped accept operation using the Windows
     * AcceptEx API, which allows the server to accept incoming client
     * connections asynchronously through the I/O Completion Port (IOCP)
     * associated with the server socket. The AcceptEx function pointer is
     * retrieved once using a static lambda and cached for subsequent calls to
     * avoid repeated WSAIoctl overhead.
     *
     * @param accept_context Reference to an AcceptContext structure that will
     * be populated with the client socket and overlapped I/O information. This
     * context must remain valid until the accept operation completes and is
     *                       processed by the IOCP worker thread.
     *
     * @return true if the AcceptEx operation was successfully posted (either
     * completed immediately or is pending), false otherwise
     *
     * @note Prerequisites:
     *       - m_server_socket must be initialized, valid, and associated with
     * an IOCP
     *       - m_accept_ctx must be allocated
     *       - The accept_context parameter must remain valid until the
     * operation completes
     *
     * @note The function creates a new client socket (TCP/IPv4) and stores it
     * in accept_context. If the operation fails, the client socket is
     * automatically closed and set to INVALID_SOCKET.
     *
     * @note The AcceptEx call is configured with:
     *       - No initial receive data (0 bytes)
     *       - Address buffers sized at sizeof(m_server_address) + 16 bytes each
     * for local and remote addresses (the +16 is required by AcceptEx for
     * internal metadata)
     *       - Overlapped I/O for asynchronous operation
     *
     * @note Error cases that return false:
     *       - Server socket is NULL
     *       - Accept context object (m_accept_ctx) is not created
     *       - AcceptEx function pointer retrieval fails (SOCKET_ERROR)
     *       - AcceptEx call fails with an error other than WSA_IO_PENDING
     *
     * @note On success, the completion notification will be posted to the IOCP
     * associated with m_server_socket, where it should be handled by calling
     * GetAcceptExSockaddrs to extract the client and server addresses.

     * @see MSDN documentation for AcceptEx and overlapped I/O operations
     */

    bool postAcceptExEvent(AcceptContext &accept_context)
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
                       "Tried to post an accept event to IOCP, but the accept "
                       "context object "
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

    /**
     * @brief Extracts the remote client's IP address and port from an AcceptEx
     * context
     *
     * This function parses the address information stored in the AcceptContext
     * buffer after an AcceptEx operation completes. It uses
     * GetAcceptExSockaddrs to retrieve both local and remote socket addresses,
     * then extracts the client's IP and port.
     *
     * @param accept_context Reference to the AcceptContext containing the
     * address buffer populated by AcceptEx
     *
     * @return std::pair<int, std::string> A pair containing:
     *         - first: The client's port number (in host byte order)
     *         - second: The client's IP address as a string (IPv4 format)
     *
     * @note This function assumes IPv4 addresses (AF_INET)
     * @note The AcceptContext's rcv_buffer must have been properly initialized
     *       and filled by a successful AcceptEx call
     */
    std::pair<int, std::string> getRemoteAddressFromAcceptContext(AcceptContext &accept_context)
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

    /**
     * @brief Posts an asynchronous receive operation for a connected client
     *
     * Initiates an overlapped WSARecv operation to receive data from the client
     * socket. The operation completes asynchronously and will be handled by the
     * I/O completion port.
     *
     * @param client Reference to the Client object whose socket will receive
     * data
     *
     * @return true if the receive operation was successfully posted (or is
     * pending)
     * @return false if WSARecv failed with an error other than WSA_IO_PENDING
     *
     * @note The function returns true even when WSA_IO_PENDING is returned by
     * WSARecv, as this indicates the operation was successfully queued
     * @note Errors are logged to the Logger instance with NETWORK type and
     * ERROR severity
     * @note The client's read overlapped structure is zeroed before use
     */
    void postReceiveEvent(Client &client)
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

        // TODO: This can end immediately if it already has data, we need to manage
        // pushing to queue and recursive call.
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

    void postSendEvent(Client &client, std::string message)
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

    /**
     * @brief Closes the socket
     */
    void closeSocket()
    {
        closesocket(m_server_socket);
        WSACleanup();
    }
};

} // namespace pulse::net
