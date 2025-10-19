#include "NetworkMessageQueue.h"

namespace pulse::net
{

NetworkMessageQueue::NetworkMessageQueue()
{
}

void NetworkMessageQueue::push(std::unique_ptr<Request> &&request)
{
    {
        std::lock_guard lock(m_mtx);
        m_queue.push(std::move(request));
    }

    m_condition_var.notify_one();
}

std::unique_ptr<Request> NetworkMessageQueue::pop()
{
    std::unique_lock lock(m_mtx);
    m_condition_var.wait(lock, [this]() { return !m_queue.empty(); });

    std::unique_ptr<Request> r = std::move(m_queue.front());
    m_queue.pop();

    return r;
}

} // namespace pulse::net
