#include "DBManager.h"
#include <stdexcept>
#include <iostream>
#include <libpq-fe.h>

DBManager::DBManager(DatabaseKeys config) {
  m_db_config = config;
  m_db_conn =
      DatabaseConnection(m_db_config.database_host, m_db_config.database_port,
                         m_db_config.database_password);
}

void DBManager::connectToDb() {

  bool should_create_db = m_db_conn.connect();

  if (should_create_db) {
    createPulseNetDB();
    m_db_conn.connect();
    if (m_db_conn.isConnected()) {
      std::cout << "Connected to database successfully\n";
    }
  }
}

bool DBManager::isConnected() { return m_db_conn.isConnected(); }

void DBManager::init() {
  createNodeEntityTable();
  createChannelTable();
  createChannelNodeTable();
}

void DBManager::createPulseNetDB() {
  std::string query = "CREATE DATABASE pulsenet;";
  PGresult *res = PQexecParams(m_db_conn.getConn(), query.c_str(), 0, NULL,
                               NULL, NULL, NULL, 0);
  ExecStatusType a = PQresultStatus(res);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::string error_message = PQerrorMessage(m_db_conn.getConn());
    std::cerr << error_message + '\n';
  }
}

void DBManager::createNodeEntityTable() {
  std::string query = "CREATE TABLE IF NOT EXISTS tbl_node_entity ("
                      "id SERIAL PRIMARY KEY, "
                      "identifier VARCHAR(50) NOT NULL UNIQUE,"
                      "password VARCHAR(255) NOT NULL,"
                      "status VARCHAR(50) NOT NULL, "
                      "ip_address INET NOT NULL"
                      ");";

  PGresult *res = PQexecParams(m_db_conn.getConn(), query.c_str(), 0, NULL,
                               NULL, NULL, NULL, 0);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::string error_message = PQerrorMessage(m_db_conn.getConn());
    std::cerr << error_message + '\n';
  }
}

void DBManager::createChannelTable() {
  std::string query = "CREATE TABLE IF NOT EXISTS tbl_channel ("
                      "id SERIAL PRIMARY KEY "
                      ");";
  PGresult *res = PQexecParams(m_db_conn.getConn(), query.c_str(), 0, NULL,
                               NULL, NULL, NULL, 0);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::string error_message = PQerrorMessage(m_db_conn.getConn());
    std::cerr << error_message + '\n';
  }
}

void DBManager::createChannelNodeTable() {
  std::string query =
      "CREATE TABLE IF NOT EXISTS tbl_channel_nodes ("
      "id SERIAL PRIMARY KEY,"
      "channel_id INT REFERENCES tbl_channel(id) ON DELETE CASCADE, "
      "node_entity_id INT REFERENCES tbl_node_entity(id) ON DELETE CASCADE"
      ");";
  PGresult *res = PQexecParams(m_db_conn.getConn(), query.c_str(), 0, NULL,
                               NULL, NULL, NULL, 0);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::string error_message = PQerrorMessage(m_db_conn.getConn());
    std::cerr << error_message + '\n';
  }
}
