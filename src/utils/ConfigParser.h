#pragma once
#include <fstream>
#include <string>
#include <map>
#include <iostream>
#include "ConfigStructures.h"

class ConfigParser {
public:
  ConfigParser();

  void read();
  DatabaseKeys getDatabaseConfig() const;

  // OVERLOADED OPERATORS
  friend std::ostream &operator<<(std::ostream &os, const ConfigParser &cp);

private:
  // CONSTANTS
  static const std::string FILE_NAME;
  static const char COMMENT_CHAR;
  static const char CONFIG_KEY_SEPARATOR;
  static const DatabaseKeys DB_KEYS;
  ;

  // PRIVATE ATTRIBUTES
  std::ifstream m_config_file;
  std::map<std::string, std::string> m_config;
};
