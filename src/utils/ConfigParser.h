#pragma once
#include <fstream>
#include <string>
#include <map>
#include <iostream>
#include "ConfigStructures.h"

/**
 * @class ConfigParser
 * @brief A class that parses config.
 *
 * This class reads and parses the configuration file storing all the params.
 * Provides methods for retrieveng params related to different parts of the
 * application like database credentials, cache parameters...
 */
class ConfigParser {
public:
  /* ----------------
   * Constructors
   * ----------------
   */
  ConfigParser();

  /* ----------------
   * Public methods
   * ----------------
   */

  /**
   * @brief This method reads the configuration file config.conf located in
   *		the application directory
   */
  void read();

  /**
   * @brief This method returns the config params related to database.
   * @return DatabaseKeys struct containing DB hostname, port and password.
   */
  DatabaseKeys getDatabaseConfig() const;

  /**
   * @return Config file location
   */
  std::string getConfigFilePath() const;

  /* ----------------
   * Overloaded operators
   * ----------------
   */
  friend std::ostream &operator<<(std::ostream &os, const ConfigParser &cp);

private:
  /* ----------------
   * Private constants
   * ----------------
   */
  static const std::string CONFIG_FILE_NAME;
  static const char COMMENT_CHAR;
  static const char CONFIG_KEY_SEPARATOR;
  static const DatabaseKeys DB_KEYS;

  /* ----------------
   * Private attributes
   * ----------------
   */
  std::ifstream m_config_file;
  std::map<std::string, std::string> m_config;

  /* ----------------
   * Private methods
   * ----------------
   */

  void createConfigFileIfNotExists(const std::string &configPath);
};
