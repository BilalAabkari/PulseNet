#pragma once

#include <string>
#ifdef _WIN32
#include <winsock2.h>
#endif

#include "constants.h"

struct Connection
{
    std::string ipAddress;
    int port;

    char recv_buffer[MAX_BUFFER_LENGHT_FOR_REQUESTS];

#ifdef _WIN32
    SOCKET sock;
    OVERLAPPED recv_overlapped;
#else
    int sock;
#endif
};
