#pragma once
#include "NetworkPlatform.h"
#include "constants.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

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
    uint64_t id;
    std::string ip_address;
    int port;
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

    /**
     * @brief Gets a pointer to the receive buffer.
     *
     * Returns a direct pointer to the internal receive buffer where incoming
     * data from the client is stored. This buffer is populated by recv() calls.
     *
     * @return char* Pointer to the receive buffer array
     *
     * @warning The returned pointer provides mutable access to the internal
     * buffer. Any modifications through this pointer will directly affect the
     *          client's internal state.
     *
     * @note Only the first GetRecvLength() bytes contain valid data.
     * @note Buffer size is MAX_BUFFER_LENGHT_FOR_REQUESTS bytes.
     *
     * @see GetRecvLength() to determine how many bytes are valid
     * @see GetRecvLengthPtr() to update the length after recv() operations
     */
    char *GetRecvBuffer()
    {
        return m_recv_buffer;
    }

    /**
     * @brief Gets a pointer to the send buffer.
     *
     * Returns a direct pointer to the internal send buffer used for preparing
     * outgoing data to be sent to the client. Data should be written to this
     * buffer before calling send().
     *
     * @return char* Pointer to the send buffer array
     *
     * @warning The returned pointer provides mutable access to the internal
     * buffer. Any modifications through this pointer will directly affect the
     *          client's internal state.
     *
     * @note Only the first GetSendLength() bytes are considered valid for
     * sending.
     * @note Buffer size is MAX_BUFFER_LENGHT_FOR_REQUESTS bytes.
     *
     * @see GetSendLength() to determine how many bytes to send
     * @see GetSendLengthPtr() to update the length after preparing data
     */
    char *GetSendBuffer()
    {
        return m_send_buffer;
    }

    /**
     * @brief Gets the number of valid bytes in the receive buffer.
     *
     * Returns the count of bytes that were received in the last recv()
     * operation and are currently valid in the receive buffer.
     *
     * @return int Number of valid bytes in m_recv_buffer
     *         - > 0: Number of bytes available to read
     *         - 0: No data received yet, or connection closed gracefully
     *
     * @note This is a read-only accessor. Use GetRecvLengthPtr() to modify the
     * length.
     * @note Valid data range: [0, MAX_BUFFER_LENGHT_FOR_REQUESTS]
     *
     * @see GetRecvBuffer() to access the actual data
     */
    int GetRecvLength() const
    {
        return m_recv_len;
    }

    /**
     * @brief Gets the number of valid bytes in the send buffer.
     *
     * Returns the count of bytes that are prepared in the send buffer and
     * ready to be transmitted, or the number of bytes successfully sent in
     * the last send() operation.
     *
     * @return int Number of valid bytes in m_send_buffer
     *         - > 0: Number of bytes to send or sent
     *         - 0: No data prepared or all data sent
     *
     * @note This is a read-only accessor. Use GetSendLengthPtr() to modify the
     * length.
     * @note Valid data range: [0, MAX_BUFFER_LENGHT_FOR_REQUESTS]
     *
     * @see GetSendBuffer() to access the buffer for writing data
     */
    int GetSendLength() const
    {
        return m_send_len;
    }

    /**
     * @brief Gets a pointer to the receive length variable.
     *
     * Returns a pointer to the internal variable tracking the number of valid
     * bytes in the receive buffer. This is primarily used to update the length
     * directly after recv() operations.
     *
     * @return int* Pointer to m_recv_len for direct modification
     *
     * @warning Provides direct mutable access to the length variable. Incorrect
     *          usage can lead to buffer overruns or reading invalid data.
     *
     * @note Typical usage: `*client->GetRecvLengthPtr() = recv(...);`
     * @note Always ensure the value stays within [0,
     * MAX_BUFFER_LENGHT_FOR_REQUESTS]
     *
     * @see GetRecvLength() for read-only access to the length value
     *
     * Example:
     * @code
     * int bytes = recv(socket, client->GetRecvBuffer(), MAX_SIZE, 0);
     * *client->GetRecvLengthPtr() = bytes;  // Update length
     * @endcode
     */
    int *GetRecvLengthPtr()
    {
        return &m_recv_len;
    }

    /**
     * @brief Gets a pointer to the send length variable.
     *
     * Returns a pointer to the internal variable tracking the number of valid
     * bytes in the send buffer. This is primarily used to update the length
     * after preparing data or after send() operations.
     *
     * @return int* Pointer to m_send_len for direct modification
     *
     * @warning Provides direct mutable access to the length variable. Incorrect
     *          usage can lead to sending incorrect amounts of data.
     *
     * @note Typical usage: `*client->GetSendLengthPtr() = data_size;`
     * @note Always ensure the value stays within [0,
     * MAX_BUFFER_LENGHT_FOR_REQUESTS]
     *
     * @see GetSendLength() for read-only access to the length value
     *
     */
    int *GetSendLengthPtr()
    {
        return &m_send_len;
    }

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

    bool isDisconnecting();

    void showInfo(std::ostream &os) const;

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
    bool m_is_disconnecting;

#ifdef _WIN32
    OVERLAPPED m_recv_overlapped{};
    OVERLAPPED m_send_overlapped{};
#endif
};

} // namespace pulse::net
