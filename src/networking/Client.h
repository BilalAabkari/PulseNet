#pragma once
#include "NetworkPlatform.h"
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#endif

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

    /* ----------------
     * Getters
     * ----------------
     */
    uint64_t getId();

    /* ----------------
     * Public methods
     * ----------------
     */

  private:
    /* ----------------
     * Private constants
     * ----------------
     */
    uint64_t m_id;
    int m_port;
    std::string m_ipAddress;
    std::atomic<int> m_reference_count;
    SOCKET_TYPE m_sock;
};
