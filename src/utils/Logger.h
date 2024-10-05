#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

enum class LogType { NETWORK, DATABASE, APPLICATION };
enum class LogSeverity { LOG_INFO, LOG_WARNING, LOG_ERROR };

class Logger {
public:
  /* ----------------
   * Public methods
   * ----------------
   */

  /**
   * @return the singleton instance
   */
  static Logger &getInstance();

  /**
   * @brief Logs a message
   */
  void log(LogType type, LogSeverity severity, const std::string &message);

  /* ----------------
   * Deleted methods for singleton pattern
   * ----------------
   */
  Logger(const Logger &) = delete;
  void operator=(const Logger &) = delete;

private:
  /* ----------------
   * Constructors
   * ----------------
   */
  Logger();
  Logger::~Logger();

  /* ----------------
   * Private methods
   * ----------------
   */
  std::string getLogDirectory() const;
  std::string getSeverityString(LogSeverity severity) const;
  std::string getCurrentTime() const;

  /* ----------------
   * Private attributes
   * ----------------
   */
  std::ofstream m_networkLog;
  std::ofstream m_dbLog;
  std::ofstream m_appLog;
  std::mutex m_mutex;
};
