#pragma once
#include "constants.h"
#include <string>

/**
 * @class NetworkManager
 * @brief A class that manages networking and socket listeners.
 *
 * This class manages the setup, control of network sockets and listeners.
 */

class NetworkManager {
public:
  /* ----------------
   * Constructors
   * ----------------
   */
  NetworkManager(int port, std::string ip_address = ANY_IP);

  /* ----------------
   * Public methods
   * ----------------
   */

  /**
   * @brief Creates the network socket
   */
  void setupSocket();

  /**
   * @brief Closes the socket
   */
  void closeSocket();

  /**
   * @brief Starts the socket listener
   */
  void startListening();

private:
  /* ----------------
   * Private attributes
   * ----------------
   */
  int m_server_socket;
  std::string m_ip_address;
  int m_port;
};
