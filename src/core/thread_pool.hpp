#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);

    // Get the number of threads in the pool
    size_t getThreadCount() const { return workers.size(); }

    // Get the number of tasks currently in the queue
    size_t getQueueSize() const;

    // Check if the thread pool is stopping
    bool isStopping() const { return stop; }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    mutable std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};
