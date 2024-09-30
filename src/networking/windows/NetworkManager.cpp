#include <iostream>
#include <winsock2.h>
#include "../NetworkManager.h"

NetworkManager::NetworkManager(int port, std::string ip_address) {

  m_ip_address = ip_address;
  m_port = port;
}

void NetworkManager::setupSocket() {
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    std::cerr << "WSAStartup failed!\n";
  }

  m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_server_socket == INVALID_SOCKET) {
    std::cerr << "Error when creating socket\n";
  }

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(m_port);
  serverAddress.sin_addr.s_addr =
      m_ip_address == ANY_IP ? INADDR_ANY : inet_addr(m_ip_address.c_str());

  int bindResult = bind(m_server_socket, (struct sockaddr *)&serverAddress,
                        sizeof(serverAddress));
  if (bindResult < 0) {
    std::cerr << "Binding socket failed.\n";
    closeSocket();
  }
}

void NetworkManager::closeSocket() {
  closesocket(m_server_socket);
  WSACleanup();
}

void NetworkManager::startListening() {
  if (listen(m_server_socket, MAX_CONNECTION_QUEUE) == SOCKET_ERROR) {
    std::cerr << "Error trying to listen the socket";
  }
}
