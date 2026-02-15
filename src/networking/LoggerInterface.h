#pragma once
#include <iostream>
#include <string_view>

namespace pulse::net
{

enum class SEVERITY : int
{
    TRACE = 0,
    S_DEBUG = 1,
    INFO = 2,
    WARN = 3,
    S_ERROR = 4,
    FATAL = 5,
    OFF = 6
};

class LoggerInterface
{
  public:
    virtual ~LoggerInterface() = default;

    virtual void write(SEVERITY severity, const std::string_view &message) const
    {
        if (!isEnabled(severity))
            return;

        std::cout << color(severity) << "[" << toString(severity) << "] " << message << COLOR_RESET << "\n";
    };

    virtual void setLevel(SEVERITY level)
    {
        m_log_level = level;
    }

  protected:
    inline const char *toString(SEVERITY s) const
    {
        switch (s)
        {
        case SEVERITY::TRACE:
            return "TRACE";
        case SEVERITY::S_DEBUG:
            return "DEBUG";
        case SEVERITY::INFO:
            return "INFO";
        case SEVERITY::WARN:
            return "WARN";
        case SEVERITY::S_ERROR:
            return "ERROR";
        case SEVERITY::FATAL:
            return "FATAL";
        case SEVERITY::OFF:
            return "OFF";
        default:
            return "UNKNOWN";
        }
    }

  private:
    SEVERITY m_log_level = SEVERITY::S_ERROR;

    inline bool isEnabled(SEVERITY msg) const
    {
        return static_cast<int>(msg) >= static_cast<int>(m_log_level);
    }

    inline const char *color(SEVERITY s) const
    {
        switch (s)
        {
        case SEVERITY::TRACE:
            return "\033[90m"; // gray
        case SEVERITY::S_DEBUG:
            return "\033[36m"; // cyan
        case SEVERITY::INFO:
            return "\033[32m"; // green
        case SEVERITY::WARN:
            return "\033[33m"; // yellow
        case SEVERITY::S_ERROR:
            return "\033[31m"; // red
        case SEVERITY::FATAL:
            return "\033[41m\033[97m"; // white on red bg
        case SEVERITY::OFF:
            return "";
        default:
            return "";
        }
    }

    static constexpr const char *COLOR_RESET = "\033[0m";
};

}; // namespace pulse::net
