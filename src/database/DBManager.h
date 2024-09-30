#pragma once

#include "DatabaseConnection.h"
#include "../utils/ConfigStructures.h"

class DBManager {

public:
  DBManager(const DatabaseKeys &config);

  void connectToDb();
  bool isConnected();
  void init();

private:
  DatabaseConnection m_db_conn;
  DatabaseKeys m_db_config;

  void createNodeEntityTable();
  void createChannelTable();
  void createChannelNodeTable();
  void createPulseNetDB();
  void executeRawSQLQuery(const std::string &query);
};
