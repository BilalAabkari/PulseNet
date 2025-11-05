#pragma once

#include "Client.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

namespace pulse::net
{

template <typename T> class ThreadSafeQueue
{
  public:
    ThreadSafeQueue()
    {
    }

    void push(std::unique_ptr<T> &&item)
    {
        {
            std::lock_guard lock(m_mtx);
            m_queue.push(std::move(item));
        }

        m_condition_var.notify_one();
    }

    std::unique_ptr<T> pop()
    {
        std::unique_lock lock(m_mtx);
        m_condition_var.wait(lock, [this]() { return !m_queue.empty(); });

        std::unique_ptr<T> r = std::move(m_queue.front());
        m_queue.pop();

        return r;
    }

  private:
    std::queue<std::unique_ptr<T>> m_queue;
    std::mutex m_mtx;
    std::condition_variable m_condition_var;
};
} // namespace pulse::net
