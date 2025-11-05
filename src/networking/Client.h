#pragma once
#include "NetworkPlatform.h"
#include "constants.h"
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace pulse::net
{

/**
 * @struct ClientDto
 * @brief Data Transfer Object for client connection information.
 *
 * A lightweight, platform-independent structure that encapsulates the essential
 * identifying information of a client connection. Used for transferring client
 * data between components without exposing the full Client implementation
 * details or platform-specific socket handles.
 *
 * @note This structure contains only the core identifying information and does
 *       not include any connection state, buffers, or socket handles.
 *
 * @see Client for the full client connection management class
 */
struct ClientDto
{
    uint64_t id{0};         ///< Unique identifier of the client
    std::string ip_address; ///< Client's IP address
    int port{0};            ///< Client's port number
};

/**
 * @class Client
 * @brief A class that parses config.
 *
 * This class maintains the state of a client and provides
 * functionality to interact with a server or network service.
 * It can manage connection details and send requests.
 */
class Client
{

  public:
    /* ----------------
     * Constructors
     * ----------------
     */
    Client(uint64_t id, int port, std::string ipAddress, SOCKET_TYPE sock);

    Client(const Client &client) = delete;
    Client(Client &&client) = delete;
    Client &operator=(const Client &client) = delete;
    Client &operator=(Client &&client) = delete;

    /* ----------------
     * Getters
     * ----------------
     */
    uint64_t getId() const;

    void addBytesReceived(int bytes);

    int getLastBytesReceived() const;

    std::pair<int, std::string> getAddress() const;

    /**
     * @brief Gets the socket descriptor/handle for this client connection.
     *
     * Returns the platform-specific socket identifier used for all network I/O
     * operations with this client. This socket is established when the client
     * connects and remains valid until the connection is closed.
     *
     * @return SOCKET_TYPE The socket descriptor
     *         - Windows: SOCKET handle (unsigned int)
     *         - Unix/Linux: File descriptor (int, typically >= 0)
     *         - INVALID_SOCKET: Socket is not valid or has been closed
     *
     * @note The socket is owned by the Client object. The caller should not
     *       close this socket directly; use the Client's disconnect/close
     * methods.
     *
     * @warning Do not store this value long-term as the socket may become
     * invalid if the client disconnects or the connection is closed.
     *
     */
    SOCKET_TYPE getSocket() const;

    int getReferenceCount() const;

#ifdef _WIN32
    /**
     * @brief Gets a pointer to the receive overlapped structure.
     *
     * Returns a pointer to the internal OVERLAPPED structure used for
     * asynchronous receive operations with WSARecv. This structure tracks
     * the state of pending I/O operations when using I/O Completion Ports.
     *
     * @return OVERLAPPED* Pointer to the receive overlapped structure
     *
     * @warning The returned pointer is valid only while the Client object
     *          exists. Do not store this pointer beyond the client's lifetime.
     *
     * @warning Do not modify the OVERLAPPED structure manually while an
     *          I/O operation is pending, as this can corrupt the operation
     * state.
     *
     * @note This overlapped structure should be used exclusively for receive
     *       operations. Use getSendOverlapped() for send operations.
     *
     * @note The structure must be zero-initialized before first use and
     *       should not be reused until the previous operation completes
     *
     * @see getSendOverlapped() for send operations
     * @see GetRecvBuffer() for the receive buffer
     */
    OVERLAPPED *getReadOverlapped();

    /**
     * @brief Gets a pointer to the send overlapped structure.
     *
     * Returns a pointer to the internal OVERLAPPED structure used for
     * asynchronous send operations with WSASend. This structure tracks
     * the state of pending I/O operations when using I/O Completion Ports.
     *
     * @return OVERLAPPED* Pointer to the send overlapped structure
     *
     * @warning The returned pointer is valid only while the Client object
     *          exists. Do not store this pointer beyond the client's lifetime.
     *
     * @warning Do not modify the OVERLAPPED structure manually while an
     *          I/O operation is pending, as this can corrupt the operation
     * state.
     *
     * @note This overlapped structure should be used exclusively for send
     *       operations. Use getReadOverlapped() for receive operations.
     *
     * @note The structure must be zero-initialized before first use and
     *       should not be reused until the previous operation completes
     *
     * @see getReadOverlapped() for receive operations
     * @see GetSendBuffer() for the send buffer
     */
    OVERLAPPED *getSendOverlapped();
#endif

    /* ----------------
     * Public methods
     * ----------------
     */
    void increaseReferenceCount();

    void decreaseReferenceCount();

    void disconnect();

    bool isDisconnecting() const;

    void showInfo(std::ostream &os) const;

    /**
     * @brief Buffer for receiving data from the client.
     *
     * Fixed-size buffer that stores incoming data from recv() operations.
     * Size defined by MAX_BUFFER_LENGHT_FOR_REQUESTS to accommodate
     * the maximum expected request size.
     *
     * @note Buffer contents are only valid up to m_recv_len bytes.
     * @warning Not null-terminated by default; treat as binary data.
     */
    char m_recv_buffer[MAX_BUFFER_LENGHT_FOR_REQUESTS];

    /**
     * @brief Buffer for sending data to the client.
     *
     * Fixed-size buffer used to prepare and store outgoing data before
     * send() operations. Size defined by MAX_BUFFER_LENGHT_FOR_REQUESTS
     * to match the maximum request/response size.
     *
     * @note Buffer contents are only valid up to m_send_len bytes.
     */
    char m_send_buffer[MAX_BUFFER_LENGHT_FOR_REQUESTS];

    int m_recv_len;
    int m_send_len;

    bool m_is_sending;

    std::queue<std::string> m_outbound_message_queue;

    std::mutex m_send_mtx;

  private:
    /* ----------------
     * Private attrbutes
     * ----------------
     */

    /**
     * @brief Unique identifier assigned to this client connection.
     *
     * This ID is generated by the server's AUTOGENERATED_ID counter when the
     * client connects. Used as the key in the server's m_client_list map and
     * for logging/debugging purposes to track individual client sessions.
     *
     * @note This ID is immutable once assigned and remains valid for the
     *       lifetime of the client connection.
     */
    uint64_t m_id;

    /**
     * @brief Port number of the remote client endpoint.
     *
     * Stores the source port used by the client for this connection.
     */
    int m_port;

    /**
     * @brief IP address of the remote client.
     *
     * String representation of the client's IP address (e.g., "192.168.1.100").
     * Extracted from the socket address information upon connection acceptance.
     */
    std::string m_ipAddress;

    /**
     * @brief Atomic reference counter for managing client lifetime.
     *
     * Tracks the number of active references to this client object across
     * different parts of the system. Used to implement safe deletion:
     * - Incremented when a new reference is created (e.g., passing to a worker
     * thread or to IOCP)
     * - Decremented when a reference is released
     * - Client can only be safely deleted when count reaches zero
     *
     * Prevents premature deletion while operations are still in progress.
     *
     * @note Atomic to ensure thread-safety without explicit locking when
     *       multiple threads handle the same client.
     */
    std::atomic<int> m_reference_count;

    /**
     * @brief Socket descriptor/handle for this client connection.
     *
     * Platform-specific socket type (SOCKET on Windows, int on Unix-like
     * systems). Used for all I/O operations (send/recv) with this particular
     * client. Must be properly closed during cleanup to prevent resource leaks.
     */
    SOCKET_TYPE m_sock;

    int m_last_bytes_received;

    /**
     * @brief Indicates whether the client is in the process of disconnecting.
     *
     * This flag is set to true when a disconnection has been detected.
     *
     * While isDisconnecting is true, no new I/O operations should be initiated
     * for this socket. The flag helps prevent race conditions and dereferences.
     *
     * Once all pending I/O operations for this client have completed
     * (pendingOps == 0), the socket can be safely closed and the client context
     * freed.
     */
    std::atomic<bool> m_is_disconnecting;

#ifdef _WIN32
    OVERLAPPED m_recv_overlapped{};
    OVERLAPPED m_send_overlapped{};
#endif
};

} // namespace pulse::net
