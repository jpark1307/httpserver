#include "httpserver/ThreadPool.hpp"

#include <spdlog/spdlog.h>

namespace httpserver {

ThreadPool::ThreadPool(size_t numThreads) {
  if (numThreads == 0) {
    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) {
      numThreads = 4; // Fallback default
    }
  }

  spdlog::info("Creating thread pool with {} workers", numThreads);

  workers_.reserve(numThreads);
  for (size_t i = 0; i < numThreads; ++i) {
    workers_.emplace_back(&ThreadPool::workerThread, this);
  }
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::workerThread() {
  spdlog::debug("Worker thread started");

  while (true) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

      if (stop_ && tasks_.empty()) {
        spdlog::debug("Worker thread exiting");
        return;
      }

      task = std::move(tasks_.front());
      tasks_.pop();
    }

    task();
  }
}

size_t ThreadPool::queueSize() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return tasks_.size();
}

void ThreadPool::shutdown() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_) {
      return; // Already shut down
    }
    stop_ = true;
  }

  condition_.notify_all();

  for (auto &worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  spdlog::info("Thread pool shut down");
}

} // namespace httpserver
