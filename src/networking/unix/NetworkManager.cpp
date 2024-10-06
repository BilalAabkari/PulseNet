#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../utils/Logger.h"
#include "../NetworkManager.h"

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

std::string NetworkManager::getIp() const { return m_ip_address; }

int NetworkManager::getPort() const { return m_port; }

void NetworkManager::closeSocket() { close(m_server_socket); }

void NetworkManager::startListening(
    const std::function<void(char *, int)> &callback, bool async) {
  if (listen(m_server_socket, MAX_CONNECTION_QUEUE) < 0) {
    throw std::runtime_error("Listening failed");
  }

  m_listening = true;
  if (async) {
    m_listener_thread =
        std::thread(&NetworkManager::startListenerLoop, this, callback);
    m_listener_thread.join();
  } else {
    startListenerLoop(callback);
  }
}

void NetworkManager::startListenerLoop(
    const std::function<void(char *, int)> &callback) {

  Logger &logger = Logger::getInstance();

  int addrlen = sizeof(m_server_address);
  int client_socket;
  char buffer[MAX_BUFFER_LENGHT_FOR_REQUESTS] = {0};

  while (m_listening) {

    client_socket =
        accept(m_server_socket, (struct sockaddr *)&m_server_address,
               (socklen_t *)&addrlen);

    if (client_socket < 0) {
      m_listening = false;
      throw std::runtime_error("Accept failed");
    }

    int bytes_received =
        read(client_socket, buffer, MAX_BUFFER_LENGHT_FOR_REQUESTS);

    std::pair<std::string, int> remoteAddressInfo =
        getRemoteAddress(client_socket);

    std::string remote_ip_address = remoteAddressInfo.first;
    int remote_port = remoteAddressInfo.second;

    logger.log(LogType::NETWORK, LogSeverity::LOG_INFO,
               "Recieved connection from " + remote_ip_address + ":" +
                   std::to_string(remote_port));

    callback(buffer, bytes_received);
  }
}

std::pair<std::string, int>
NetworkManager::getRemoteAddress(int &remoteSocket) {

  Logger &logger = Logger::getInstance();
  sockaddr_in remoteAddress;
  socklen_t remote_addr_len = sizeof(remoteAddress);
  int result = getpeername(remoteSocket, (struct sockaddr *)&remoteAddress,
                           &remote_addr_len);

  if (result < 0) {
    throw std::runtime_error("Error getting remote address");
  }

  char ip_address[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &remoteAddress.sin_addr, ip_address, sizeof(ip_address));
  int port = ntohs(remoteAddress.sin_port);

  return {ip_address, port};
}
