#pragma once

#include "Client.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

namespace pulse::net
{
struct Request
{
    ClientDto client;
    std::string message;
};

class NetworkMessageQueue
{
  public:
    NetworkMessageQueue();
    void push(std::unique_ptr<Request> &&request);
    std::unique_ptr<Request> pop();

  private:
    std::queue<std::unique_ptr<Request>> m_queue;
    std::mutex m_mtx;
    std::condition_variable m_condition_var;
};
} // namespace pulse::net
