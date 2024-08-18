#include "ConfigParser.h"

const std::string FILE_NAME = "config.cong";
const char COMMENT_CHAR = '*';
const char CONFIG_KEY_SEPARATOR = '=';

ConfigParser::ConfigParser() {
  m_config_file.open(FILE_NAME);
  if (!m_config_file.is_open()) {
    throw std::runtime_error("Cound't find the configuration file");
  }
}

void ConfigParser::read() {
  std::string line;
  while (std::getline(m_config_file, line)) {
    if (!line.empty() && line[0] != COMMENT_CHAR) {
      size_t separator_index = line.find(CONFIG_KEY_SEPARATOR);
      std::string config_key = line.substr(0, separator_index - 1);
      std::string config_value =
          line.substr(separator_index = 1, line.size() - 1);
      m_config[config_key] = config_value;
    }
  }
}
