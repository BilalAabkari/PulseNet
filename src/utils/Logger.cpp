#include "Logger.h"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

Logger &Logger::getInstance()
{
    static Logger instance; // Singleton instance
    return instance;
}

void Logger::log(LogType type, LogSeverity severity, const std::string &message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string logMessage = "[" + getCurrentTime() + "] " + getSeverityString(severity) + ": " + message;

    switch (type)
    {
    case LogType::NETWORK:
        m_networkLog << logMessage << std::endl;
        break;
    case LogType::DATABASE:
        m_dbLog << logMessage << std::endl;
        break;
    case LogType::APPLICATION:
        m_appLog << logMessage << std::endl;
        break;
    default:
        break;
    }
}

Logger::Logger()
{
    std::string logDir = getLogDirectory();
    std::filesystem::create_directories(logDir);

    m_networkLog.open(logDir + "/network.log", std::ios::app);
    m_dbLog.open(logDir + "/database.log", std::ios::app);
    m_appLog.open(logDir + "/application.log", std::ios::app);

    if (!m_networkLog || !m_dbLog || !m_appLog)
    {
        std::cerr << "Failed to open log files." << std::endl;
    }
}

Logger::~Logger()
{
    if (m_networkLog.is_open())
        m_networkLog.close();
    if (m_dbLog.is_open())
        m_dbLog.close();
    if (m_appLog.is_open())
        m_appLog.close();
}

std::string Logger::getLogDirectory() const
{

#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path)))
    {
        return std::string(path) + "\\PulseNet\\logs";
    }
    else
    {
        return "C:\\PulseNet\\logs";
    }
#else
    const char *homeDir;
    if ((homeDir = getenv("HOME")) == NULL)
    {
        homeDir = getpwuid(getuid())->pw_dir;
    }
    return std::string(homeDir) + "/.local/share/PulseNet/logs";
#endif
}

std::string Logger::getSeverityString(LogSeverity severity) const
{
    switch (severity)
    {
    case LogSeverity::LOG_INFO:
        return "INFO";
    case LogSeverity::LOG_WARNING:
        return "WARNING";
    case LogSeverity::LOG_ERROR:
        return "ERROR";
    case LogSeverity::POSTGRES_LOG:
        return "[POSTGRES LOG]";
    case LogSeverity::NONE:
        return "";
    default:
        return "UNKNOWN";
    }
}

std::string Logger::getCurrentTime() const
{
    std::time_t t = std::time(nullptr);
    std::tm now;

#ifdef _WIN32
    localtime_s(&now, &t);
#else
    localtime_r(&t, &now);
#endif

    std::ostringstream oss;
    oss << std::put_time(&now, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
