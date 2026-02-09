#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace httpserver {

/**
 * Modern C++17 thread pool with work queue.
 * Workers wait for tasks and execute them in parallel.
 */
class ThreadPool {
public:
  /**
   * Create a thread pool with the specified number of workers.
   * @param numThreads Number of worker threads (default: hardware concurrency)
   */
  explicit ThreadPool(size_t numThreads = 0);

  /**
   * Destructor joins all worker threads after completing pending tasks.
   */
  ~ThreadPool();

  // Non-copyable, non-movable
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;

  /**
   * Submit a task to the thread pool.
   * @param f Callable to execute
   * @param args Arguments to pass to the callable
   * @return Future for the result
   */
  template <typename F, typename... Args>
  auto submit(F &&f, Args &&...args)
      -> std::future<std::invoke_result_t<F, Args...>>;

  /**
   * Get the number of pending tasks in the queue.
   */
  size_t queueSize() const;

  /**
   * Get the number of worker threads.
   */
  size_t numWorkers() const { return workers_.size(); }

  /**
   * Shutdown the thread pool gracefully.
   * All pending tasks will be completed before workers exit.
   */
  void shutdown();

private:
  void workerThread();

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::atomic<bool> stop_{false};
};

// Template implementation
template <typename F, typename... Args>
auto ThreadPool::submit(F &&f, Args &&...args)
    -> std::future<std::invoke_result_t<F, Args...>> {

  using ReturnType = std::invoke_result_t<F, Args...>;

  auto task = std::make_shared<std::packaged_task<ReturnType()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<ReturnType> result = task->get_future();

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_) {
      throw std::runtime_error("submit() called on stopped ThreadPool");
    }
    tasks_.emplace([task]() { (*task)(); });
  }

  condition_.notify_one();
  return result;
}

} // namespace httpserver
