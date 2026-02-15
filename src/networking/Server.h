#pragma once

#include <string>

class Server
{
  public:
    virtual std::string getIp() const = 0;
    virtual int getPort() const = 0;

    virtual void start() = 0;
    virtual void send(uint64_t id, const std::string &message) = 0;

  private:
    int m_min_client_buffer_len;
    int m_max_client_buffer_len;
};
