#pragma once

#include "ThreadPool.h"

namespace pulse::net
{

ThreadPool::ThreadPool(int workers, std::function<void()> callback)
    : m_workers(workers), m_callback(callback), m_running(false)
{
    if (workers < 1)
    {
        throw std::invalid_argument("Workers should be at least 1");
    }
}

ThreadPool::~ThreadPool()
{
    stop();
}

void ThreadPool::run()
{
    m_running = true;

    for (int i = 0; i < m_workers; i++)
    {
        ThreadContext ctx;
        ctx.id = i;
        ctx.status = Status::IDLE;
        ctx.thread = std::thread(worker, this, i);

        m_threads.push_back(std::move(ctx));
    }
}

void ThreadPool::stop()
{
    m_running = false;
    for (auto &ctx : m_threads)
    {
        if (ctx.thread.joinable())
        {
            ctx.thread.join();
        }
    }

    m_threads.clear();
}

void ThreadPool::worker(int id)
{
    m_threads[id].status = Status::RUNNING;

    while (m_running)
    {
        m_callback();
    }

    m_threads[id].status = Status::IDLE;

} // namespace pulse::net
