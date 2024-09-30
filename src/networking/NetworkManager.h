#pragma once
#include "constants.h"
#include <string>

class NetworkManager {
public:
  NetworkManager(int port, std::string ip_address = ANY_IP);
  void setupSocket();
  void closeSocket();
  void startListening();

private:
  int m_server_socket;
  std::string m_ip_address;
  int m_port;
};
