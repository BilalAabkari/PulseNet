#include "ConfigParser.h"

const std::string ConfigParser::FILE_NAME =
    "C:\\Users\\bilal\\OneDrive\\Escritorio\\Bilal\\personal "
    "projects\\PulseNet\\src\\config.conf";
const char ConfigParser::COMMENT_CHAR = '*';
const char ConfigParser::CONFIG_KEY_SEPARATOR = '=';
const DatabaseKeys ConfigParser::DB_KEYS = {
    "DATABASE_HOST",
    "DATABASE_PORT",
    "DATABASE_PASSWORD",
};

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
      std::string config_key = line.substr(0, separator_index);
      std::string config_value =
          line.substr(separator_index + 1, line.size() - 1);
      m_config[config_key] = config_value;
    }
  }
}

DatabaseKeys ConfigParser::getDatabaseConfig() const {
  DatabaseKeys db_info = {
      m_config.at(DB_KEYS.database_host),
      m_config.at(DB_KEYS.database_port),
      m_config.at(DB_KEYS.database_password),
  };
  return db_info;
}

std::ostream &operator<<(std::ostream &os, const ConfigParser &cp) {
  std::map<std::string, std::string>::const_iterator it;
  for (it = cp.m_config.begin(); it != cp.m_config.end(); it++) {
    os << it->first << "=" << it->second << '\n';
  }

  return os;
}
