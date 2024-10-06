#pragma once
#include <string>
#include <functional>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "constants.h"

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
  void startListening(const std::function<void(char *, int)> &callback,
                      bool async = true);

  std::string getIp() const;
  int getPort() const;

private:
  /* ----------------
   * Private attributes
   * ----------------
   */

#ifdef _WIN32
  SOCKET m_server_socket;
#else
  int m_server_socket;
#endif
  std::string m_ip_address;
  sockaddr_in m_server_address;
  int m_port;
  bool m_listening;
  std::thread m_listener_thread;

  /* ----------------
   * Private methdos
   * ----------------
   */

#ifdef _WIN32
  std::pair<std::string, int> getRemoteAddress(SOCKET &remoteSocket);
#else
  std::pair<std::string, int> getRemoteAddress(int &remoteSocket);
#endif

  void startListenerLoop(const std::function<void(char *, int)> &callback);
};
