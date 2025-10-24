#pragma once

#include <functional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace pulse::net
{

class ThreadPool
{
  public:
    ThreadPool(int workers, std::function<void()> callback);

    ~ThreadPool();

    void run();

    void stop();

  private:
    enum Status
    {
        IDLE,
        RUNNING
    };

    struct ThreadContext
    {
        int id;
        std::thread thread;
        Status status;
    };

    std::vector<ThreadContext> m_threads;

    int m_workers;

    std::function<void()> m_callback;

    bool m_running;

    void worker(int id);
};

} // namespace pulse::net
