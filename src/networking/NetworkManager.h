#pragma once
#include "constants.h"
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>

#include "Client.h"
#include "NetworkMessageQueue.h"
#include "NetworkPlatform.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace pulse::net
{

/**
 * @class NetworkManager
 * @brief A class that manages networking and socket listeners.
 *
 * This class manages the setup, control of network sockets and listeners.
 */

class NetworkManager
{
  public:
    /* ----------------
     * Constructors
     * ----------------
     */
    NetworkManager(int port, std::string ip_address = ANY_IP);
    NetworkManager(const NetworkManager &nm) = delete;
    NetworkManager(const NetworkManager &&nm) = delete;
    NetworkManager &operator=(const NetworkManager &nm) = delete;
    NetworkManager &operator=(NetworkManager &&nm) = delete;

    /* ----------------
     * Public methods
     * ----------------
     */

    /**
     * @brief Creates the network socket
     */
    void setupSocket();

    /**
     * @brief Closes the socket
     */
    void closeSocket();

    /**
     *  @brief Stops de listener safely.
     */
    void stop();

    /**
     * @brief Starts the socket listener
     */
    void startListening(const std::function<void(Client &, char *)> &callback, bool async = true);

    /*
     * @brief Adds a new client to the clients list. Thread safe
     */
    Client *addClient(uint64_t id, int port, std::string ipAddress, SOCKET sock);

    /*
     * @bried Creates a request after reveiving from client
     */
    std::unique_ptr<Request> createRequest(const Client &client);

    void terminateClient(uint64_t id);

    std::string getIp() const;
    int getPort() const;

  private:
    /* ----------------
     * Private attributes
     * ----------------
     */

#ifdef _WIN32

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
    AcceptContext *m_accept_ctx;

    /**
     * @brief Atomic counter tracking the number of pending asynchronous accept
     * operations.
     *
     * Maintains a count of AcceptEx calls that have been initiated but not yet
     * completed. This is used to ensure proper cleanup during shutdown (wait for pending operations):
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
    std::atomic<int> m_pending_accepts;

#endif

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
    SOCKET_TYPE m_server_socket;

    /**
     * @brief Atomic counter for generating unique client identifiers.
     *
     * Thread-safe monotonically increasing counter used to assign unique IDs
     * to each connected client. Incremented atomically on each new connection.
     *
     * @note Atomic to ensure thread-safety without explicit locking.
     */
    std::atomic<uint64_t> AUTOGENERATED_ID;

    /**
     * @brief Atomic flag indicating whether the server is actively listening.
     *
     * Used to control the listener thread's execution and signal shutdown.
     * - true: Server is accepting connections
     * - false: Server should stop accepting new connections
     *
     * @note Atomic to allow safe checking across threads without locks.
     */
    std::atomic<bool> m_listening;

    /**
     * @brief Mutex for protecting concurrent access to the client list.
     *
     * Synchronizes operations that modify or iterate over m_client_list,
     * preventing race conditions when multiple threads access client data.
     */
    std::shared_mutex m_mtx;

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

    /* ----------------
     * Private methdos
     * ----------------
     */

    NetworkMessageQueue m_requests_queue{};

#ifdef _WIN32

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

    bool postAcceptExEvent(AcceptContext &accept_context);

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
    std::pair<int, std::string> getRemoteAddressFromAcceptContext(AcceptContext &accept_context);

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
    bool postReceiveEvent(Client &client);

#endif
};

} // namespace pulse::net
