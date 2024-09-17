#include "DatabaseConnection.h"
#include <stdexcept>

const std::string DatabaseConnection::DEFAULT_HOST = "localhost";
const std::string DatabaseConnection::DEFAULT_DATABASE_NAME = "postgres";
const std::string DatabaseConnection::PULSE_NET_DATABASE_NAME = "pulsenet_db";
const std::string DatabaseConnection::DEFAULT_PORT = "5432";
const std::string DatabaseConnection::DEFAULT_USER = "postgres";

DatabaseConnection::DatabaseConnection(std::string host, std::string port,
                                       std::string password) {
  m_host = host;
  m_port = port;
  m_password = password;
}

DatabaseConnection::~DatabaseConnection() {
  if (m_conn) {
    PQfinish(m_conn);
    m_conn = nullptr;
  }
}

bool DatabaseConnection::connect() {

  bool should_create_database = false;

  if (m_password.empty()) {
    throw std::runtime_error("Database password is not set");
  }

  m_host = m_host.empty() ? DEFAULT_HOST : m_host;
  m_port = m_port.empty() ? DEFAULT_PORT : m_port;

  std::string conninfo =
      "host = " + m_host + " port = " + m_port + " password = " + m_password +
      " dbname = " + PULSE_NET_DATABASE_NAME + " user = " + DEFAULT_USER;

  m_conn = PQconnectdb(conninfo.c_str());

  if (PQstatus(m_conn) != CONNECTION_OK) {
    // If connection failed, we try with default postgres dbname
    should_create_database = true;
    conninfo = "host = " + m_host + " port = " + m_port +
               " password = " + m_password +
               " dbname = " + DEFAULT_DATABASE_NAME + " user = " + DEFAULT_USER;
    m_conn = PQconnectdb(conninfo.c_str());
    if (PQstatus(m_conn) != CONNECTION_OK) {
      throw std::runtime_error("Couldn't connect to the database");
    }
  }

  return should_create_database;
}

bool DatabaseConnection::isConnected() const {
  return PQstatus(m_conn) == CONNECTION_OK;
}
