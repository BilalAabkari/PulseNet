#include "DBManager.h"
#include "../utils/Logger.h"
#include <stdexcept>
#include <iostream>
#include <libpq-fe.h>

DBManager::DBManager(const DatabaseKeys &config) {
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
  PGresult *res = executeRawSQLQuery(query);
  logSQLQueryResult(res, query);
}

void DBManager::createNodeEntityTable() {
  std::string query = "CREATE TABLE IF NOT EXISTS tbl_node_entity ("
                      "id SERIAL PRIMARY KEY, "
                      "identifier VARCHAR(50) NOT NULL UNIQUE,"
                      "password VARCHAR(255) NOT NULL,"
                      "status VARCHAR(50) NOT NULL, "
                      "ip_address INET NOT NULL"
                      ");";

  PGresult *res = executeRawSQLQuery(query);
  logSQLQueryResult(res, query);
}

void DBManager::createChannelTable() {
  std::string query = "CREATE TABLE IF NOT EXISTS tbl_channel ("
                      "id SERIAL PRIMARY KEY "
                      ");";
  PGresult *res = executeRawSQLQuery(query);
  logSQLQueryResult(res, query);
}

void DBManager::createChannelNodeTable() {
  std::string query =
      "CREATE TABLE IF NOT EXISTS tbl_channel_nodes ("
      "id SERIAL PRIMARY KEY,"
      "channel_id INT REFERENCES tbl_channel(id) ON DELETE CASCADE, "
      "node_entity_id INT REFERENCES tbl_node_entity(id) ON DELETE CASCADE"
      ");";

  PGresult *res = executeRawSQLQuery(query);
  logSQLQueryResult(res, query);
}

PGresult *DBManager::executeRawSQLQuery(const std::string &query) {

  return PQexecParams(m_db_conn.getConn(), query.c_str(), 0, NULL, NULL, NULL,
                      NULL, 0);
}

void DBManager::logSQLQueryResult(const PGresult *res, const std::string &query,
                                  const std::string &message) const {
  ExecStatusType status = PQresultStatus(res);
  Logger &logger = Logger::getInstance();

  switch (status) {
  case PGRES_COMMAND_OK:
  case PGRES_TUPLES_OK:
  case PGRES_COPY_OUT:
  case PGRES_COPY_IN:
  case PGRES_COPY_BOTH:
  case PGRES_SINGLE_TUPLE:
  case PGRES_PIPELINE_SYNC:
    if (!message.empty())
      logger.log(LogType::DATABASE, LogSeverity::LOG_INFO, message);
    break;
  case PGRES_EMPTY_QUERY:
    logger.log(LogType::DATABASE, LogSeverity::LOG_WARNING,
               "SQL query is empty: " + query);
    break;
  case PGRES_NONFATAL_ERROR:
    logger.log(LogType::DATABASE, LogSeverity::LOG_WARNING,
               PQerrorMessage(m_db_conn.getConn()));
    break;
  default:
    logger.log(LogType::DATABASE, LogSeverity::LOG_ERROR,
               PQerrorMessage(m_db_conn.getConn()));
    break;
  }
}
