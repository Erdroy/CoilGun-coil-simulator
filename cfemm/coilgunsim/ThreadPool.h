#pragma once

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <functional>
#include <mutex>
#include <queue>

class ThreadPool
{
public:
    void Start(uint32_t numThreads);
    void QueueJob(const std::function<void(uint32_t)>& job);
    void Stop();
    bool IsBusy();

    uint32_t GetNumJobs();

private:
    void ThreadLoop(uint32_t id);

    bool should_terminate = false;           // Tells threads to stop looking for jobs
    std::mutex queue_mutex;                  // Prevents data races to the job queue
    std::condition_variable mutex_condition; // Allows threads to wait on new jobs or termination 
    std::atomic<int> num_jobs_running;
    std::vector<std::thread> threads;
    std::queue<std::function<void(uint32_t)>> jobs;
};

#endif // THREADPOOL_H
