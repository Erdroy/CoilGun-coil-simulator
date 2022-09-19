#include "ThreadPool.h"

// source: https://stackoverflow.com/questions/15752659/thread-pooling-in-c11

void ThreadPool::ThreadLoop(const uint32_t id)
{
    while (true)
    {
        std::function<void(uint32_t)> job;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            mutex_condition.wait(lock, [this] {
                return !jobs.empty() || should_terminate;
            });
            if (should_terminate)
                return;
            job = jobs.front();
            jobs.pop();
            num_jobs_running.fetch_add(1);
        }

        job(id);
        num_jobs_running.fetch_sub(1);
    }
}

void ThreadPool::Start(const uint32_t numThreads)
{
    threads.resize(numThreads);
    for (uint32_t i = 0; i < numThreads; i++)
    {
        threads.at(i) = std::thread(&ThreadPool::ThreadLoop, this, i);
    }
}

void ThreadPool::QueueJob(const std::function<void(uint32_t)>& job)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        jobs.push(job);
    }
    mutex_condition.notify_one();
}

bool ThreadPool::IsBusy()
{
    bool busy;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        busy = jobs.empty();
    }
    return busy || num_jobs_running.load() > 0;
}

uint32_t ThreadPool::GetNumJobs()
{
    // Lock and return the size of jobs queue
    uint32_t numJobs;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        numJobs = static_cast<uint32_t>(jobs.size());
    }

    return numJobs;
}

void ThreadPool::Stop()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        should_terminate = true;
    }
    mutex_condition.notify_all();
    for (std::thread& active_thread : threads)
    {
        active_thread.join();
    }
    threads.clear();
}
