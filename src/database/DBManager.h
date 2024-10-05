#pragma once

#include "DatabaseConnection.h"
#include "../utils/ConfigStructures.h"

/**
 * @class DBManager
 * @brief A class that manages PulseNet database
 *
 * This class manmages all the database operations needed for pulsenet.
 */
class DBManager {

public:
  /* ----------------
   * Constructors
   * ----------------
   */
  DBManager(const DatabaseKeys &config);

  /* ----------------
   * Public methods
   * ----------------
   */

  /**
   * @brief Connects to the pulsenet database.
   */
  void connectToDb();

  /**
   * @brief Checks if the connetion to the database is active.
   */
  bool isConnected();

  /**
   * @brief This method initializes the setup or update of the database scheme.
   */
  void init();

private:
  /* ----------------
   * Private attributes
   * ----------------
   */
  DatabaseConnection m_db_conn;
  DatabaseKeys m_db_config;

  /* ----------------
   * Private methods
   * ----------------
   */
  void createNodeEntityTable();
  void createChannelTable();
  void createChannelNodeTable();
  void createPulseNetDB();
  PGresult *executeRawSQLQuery(const std::string &query);
  void logSQLQueryResult(const PGresult *res, const std::string &query,
                         const std::string &message = "") const;
};
