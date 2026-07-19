#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace livevel {

class ThreadManager {
public:
    ThreadManager(size_t numThreads);
    ~ThreadManager();

    // Enqueue a task
    void enqueue(std::function<void()> task);

    // Wait for all enqueued tasks to complete
    void waitAll();

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::condition_variable waitCondition_;

    std::atomic<bool> stop_;
    std::atomic<int> activeTasks_;
};

} // namespace livevel
