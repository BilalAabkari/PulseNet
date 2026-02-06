#pragma once

#include <string>

class Server
{
  public:
    virtual std::string getIp() const = 0;
    virtual int getPort() const = 0;

    virtual void start() = 0;
    virtual void send(uint64_t id, const std::string &message) = 0;

    virtual void enableLogs(bool enabled)
    {
        m_logs_enabled = enabled;
    };

  private:
    bool m_logs_enabled = false;
};
