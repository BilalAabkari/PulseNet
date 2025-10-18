#include "Client.h"

Client::Client(uint64_t id, int port, std::string ipAddress, SOCKET_TYPE sock)
    : m_id(id), m_port(port), m_ipAddress(ipAddress), m_sock(sock)
{
    m_recv_len = 0;
    m_send_len = 0;
}

uint64_t Client::getId()
{
    return m_id;
}

SOCKET_TYPE Client::getSocket()
{
    return m_sock;
}

int Client::getReferenceCount()
{
    return m_reference_count.load();
}

void Client::increaseReferenceCount()
{
    m_reference_count.fetch_add(1);
};

void Client::decreaseReferenceCount()
{
    m_reference_count.fetch_sub(1);
};

void Client::disconnect()
{
    m_is_disconnecting = true;
}

bool Client::isDisconnecting()
{
    return m_is_disconnecting;
}

#ifdef _WIN32
OVERLAPPED *Client::getSendOverlapped()
{
    return &m_send_overlapped;
}

OVERLAPPED *Client::getReadOverlapped()
{
    return &m_recv_overlapped;
}
#endif
