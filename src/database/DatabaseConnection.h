#pragma once
#include <string>
#include <libpq-fe.h>

/**
 * @class ConfigParser
 * @brief A class that manages database connections
 */
class DatabaseConnection {
public:
  /* ----------------
   * Constructors
   * ----------------
   */
  DatabaseConnection();
  DatabaseConnection(std::string host, std::string port, std::string password);
  ~DatabaseConnection();

  /* ----------------
   * Public methods
   * ----------------
   */

  /**
   * @brief Returns true if we have a connection to the database, false
   * otherwise
   */
  bool isConnected() const;

  /**
   * @brief Gets the connection object to our posgres database, which will be
   * necessary in order to execute sql queries from an external class
   * @return potsgres DB connection
   */
  PGconn *getConn() const;

  /**
   * @brief Uses the private attributes to try to connect to the database.
   * It tries to connect to a database with the name specified in
   * DatabaseConnection::PULSE_NET_DATABASE_NAME. If connection fails it means
   * that a database with that name doesn't exist, so it connects to the default
   * posgres database instead, which is named postgres.
   *
   * @return true if pulsenet database is created,
   * false otherwise. If it's false, it means that the connection was
   * successful, but to the default postgres database, so we need to create
   * pulsenet db in order to use the application.
   */
  bool connect();

private:
  /* ----------------
   * Private constants
   * ----------------
   */
  static const std::string DEFAULT_HOST;
  static const std::string DEFAULT_DATABASE_NAME;
  static const std::string PULSE_NET_DATABASE_NAME;
  static const std::string DEFAULT_PASSWORD;
  static const std::string DEFAULT_PORT;
  static const std::string DEFAULT_USER;

  /* ----------------
   * Private attributes
   * ----------------
   */
  std::string m_host;
  std::string m_database_name;
  std::string m_password;
  std::string m_port;
  PGconn *m_conn;
};
