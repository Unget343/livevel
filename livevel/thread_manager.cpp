#include "thread_manager.h"

namespace livevel {

ThreadManager::ThreadManager(size_t numThreads) : stop_(false), activeTasks_(0) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex_);
                    this->condition_.wait(lock, [this] {
                        return this->stop_ || !this->tasks_.empty();
                    });
                    if (this->stop_ && this->tasks_.empty()) {
                        return;
                    }
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }

                // Execute the task
                task();

                {
                    std::lock_guard<std::mutex> lock(this->queueMutex_);
                    this->activeTasks_--;
                    if (this->tasks_.empty() && this->activeTasks_ == 0) {
                        this->waitCondition_.notify_all();
                    }
                }
            }
        });
    }
}

ThreadManager::~ThreadManager() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread& worker : workers_) {
        worker.join();
    }
}

void ThreadManager::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        tasks_.push(std::move(task));
        activeTasks_++;
    }
    condition_.notify_one();
}

void ThreadManager::waitAll() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    waitCondition_.wait(lock, [this] {
        return tasks_.empty() && activeTasks_ == 0;
    });
}

} // namespace livevel
