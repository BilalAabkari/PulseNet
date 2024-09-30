#include "../NetworkManager.h"
#include <iostream>
#include <sys/socket.h>

NetworkManager::NetworkManager(int port, std::string ip_address) {

  m_ip_address = ip_address;
  m_port = port;
}

void NetworkManager::setupSocket() {
  m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_server_socket < 0) {
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

void NetworkManager::closeSocket() { close(m_server_socket); }

void NetworkManager::startListening() {
  if (listen(m_server_socket, MAX_CONNECTION_QUEUE) < 0) {
    throw std::runtime_error("Listening failed");
  }
}
