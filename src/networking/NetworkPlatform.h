#pragma once

#ifdef _WIN32
#include <winsock2.h>
#define SOCKET_TYPE SOCKET
#else
#include <sys/socket.h>
#define SOCKET_TYPE int
#endif
