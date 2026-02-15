#pragma once
#include "LoggerInterface.h"
#include <atomic>

namespace pulse::net
{

class LoggerManager
{
  public:
    static void set_logger(LoggerInterface *logger)
    {
        m_logger = logger;
    }

    static LoggerInterface *get_logger()
    {
        auto logger = m_logger.load();
        return logger ? logger : &m_default_logger;
    }

    static void setLevel(SEVERITY level)
    {
        get_logger()->setLevel(level);
    }

  private:
    static inline std::atomic<LoggerInterface *> m_logger{nullptr};
    static inline LoggerInterface m_default_logger;
};

} // namespace pulse::net
