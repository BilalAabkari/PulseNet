#pragma once
#include <fstream>
#include <string>
#include <map>

class ConfigParser {
public:
  ConfigParser();

  void read();

private:
  // CONSTANTS
  static const std::string FILE_NAME;
  static const char COMMENT_CHAR;
  static const char CONFIG_KEY_SEPARATOR;

  // PRIVATE ATTRIBUTES
  std::ifstream m_config_file;
  std::map<std::string, std::string> m_config;
};
