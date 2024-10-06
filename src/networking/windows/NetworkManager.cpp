#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <exception>

#include "../../utils/Logger.h"
#include "../NetworkManager.h"

NetworkManager::NetworkManager(int port, std::string ip_address) {

  m_ip_address = ip_address;
  m_port = port;
  m_server_socket = NULL;
  m_server_address.sin_family = AF_INET;
  m_server_address.sin_port = htons(m_port);
  m_server_address.sin_addr.s_addr =
      m_ip_address == ANY_IP ? INADDR_ANY : inet_addr(m_ip_address.c_str());
}

std::string NetworkManager::getIp() const { return m_ip_address; }

int NetworkManager::getPort() const { return m_port; }

void NetworkManager::setupSocket() {
  Logger &logger = Logger::getInstance();
  WSADATA wsaData;

  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, "WSAStartup failed!");

    throw std::runtime_error("WSAStartup failed!");
  }

  m_server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (m_server_socket == INVALID_SOCKET) {
    throw std::runtime_error(
        "Error when creating socket! (ip: " + m_ip_address +
        ", port: " + std::to_string(m_port));
  }

  int bindResult = bind(m_server_socket, (struct sockaddr *)&m_server_address,
                        sizeof(m_server_address));
  if (bindResult < 0) {
    throw std::runtime_error("Binding socket faile! (ip: " + m_ip_address +
                             ", port: " + std::to_string(m_port));

    closeSocket();
  }
}

void NetworkManager::closeSocket() {
  closesocket(m_server_socket);
  WSACleanup();
}

void NetworkManager::startListening(
    const std::function<void(char *, int)> &callback, bool async) {
  Logger &logger = Logger::getInstance();
  int addrlen = sizeof(m_server_address);

  if (listen(m_server_socket, MAX_CONNECTION_QUEUE) == SOCKET_ERROR) {
    throw std::runtime_error(
        "Error trying to listen the socket! (ip: " + m_ip_address +
        ", port: " + std::to_string(m_port));
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

std::pair<std::string, int>
NetworkManager::getRemoteAddress(SOCKET &remoteSocket) {

  Logger &logger = Logger::getInstance();
  sockaddr_in remoteAddress;
  int remote_addr_len = sizeof(remoteAddress);
  SOCKET result = getpeername(remoteSocket, (struct sockaddr *)&remoteAddress,
                              &remote_addr_len);

  if (result == SOCKET_ERROR) {
    closesocket(remoteSocket);
    closesocket(m_server_socket);
    WSACleanup();
    throw std::runtime_error("Error getting remote address");
  }

  char ip_address[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &remoteAddress.sin_addr, ip_address, sizeof(ip_address));
  int port = ntohs(remoteAddress.sin_port);

  return {ip_address, port};
}

void NetworkManager::startListenerLoop(
    const std::function<void(char *, int)> &callback) {

  Logger &logger = Logger::getInstance();

  int addrlen = sizeof(m_server_address);
  SOCKET client_socket;
  char buffer[MAX_BUFFER_LENGHT_FOR_REQUESTS] = {0};

  while (m_listening) {

    client_socket =
        accept(m_server_socket, (struct sockaddr *)&m_server_address, &addrlen);

    if (client_socket == SOCKET_ERROR) {
      closeSocket();
      m_listening = false;
      throw std::runtime_error("Accept failed: " + WSAGetLastError());
    }

    int bytes_received =
        recv(client_socket, buffer, MAX_BUFFER_LENGHT_FOR_REQUESTS, 0);
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
