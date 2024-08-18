#pragma once
#include <string>
#include <libpq-fe.h>

class DatabaseConnection {
public:
  DatabaseConnection(std::string host, std::string port, std::string password);

  bool isConnected() const;

  bool connect();

private:
  static const std::string DEFAULT_HOST;
  static const std::string DEFAULT_DATABASE_NAME;
  static const std::string PULSE_NET_DATABASE_NAME;
  static const std::string DEFAULT_PASSWORD;
  static const std::string DEFAULT_PORT;
  static const std::string DEFAULT_USER;

  std::string m_host;
  std::string m_database_name;
  std::string m_password;
  std::string m_port;
  PGconn *m_conn;
};
