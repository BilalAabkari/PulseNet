#include "Client.h"

Client::Client(uint64_t id, int port, std::string ipAddress, SOCKET_TYPE sock)
    : m_id(id), m_port(port), m_ipAddress(ipAddress), m_sock(sock)
{
}

uint64_t Client::getId()
{
    return m_id;
}
