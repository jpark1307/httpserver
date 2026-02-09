#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

namespace httpserver {

enum class EventType { Read, Write, ReadWrite };

struct Event {
  int fd;
  EventType type;
  void *userData;
  bool isError;
  bool isEof;
};

/**
 * Reactor-pattern event loop using kqueue (macOS) or epoll (Linux).
 * Monitors file descriptors for I/O readiness.
 */
class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  // Non-copyable, non-movable
  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;
  EventLoop(EventLoop &&) = delete;
  EventLoop &operator=(EventLoop &&) = delete;

  /**
   * Add a file descriptor to the event loop.
   * @param fd File descriptor to monitor
   * @param type Type of events to monitor
   * @param userData Optional pointer stored with the fd
   */
  void addFd(int fd, EventType type, void *userData = nullptr);

  /**
   * Modify the events monitored for a file descriptor.
   * @param fd File descriptor to modify
   * @param type New event type to monitor
   * @param userData Optional updated user data
   */
  void modifyFd(int fd, EventType type, void *userData = nullptr);

  /**
   * Remove a file descriptor from the event loop.
   * @param fd File descriptor to remove
   */
  void removeFd(int fd);

  /**
   * Wait for events to be ready.
   * @param timeoutMs Timeout in milliseconds (-1 for infinite)
   * @return Vector of ready events
   */
  std::vector<Event> wait(int timeoutMs = -1);

  /**
   * Signal the event loop to stop.
   */
  void stop();

  /**
   * Check if the event loop is still running.
   */
  bool isRunning() const { return running_.load(std::memory_order_relaxed); }

private:
  int kqueueFd_;
  std::atomic<bool> running_{true};

  static constexpr int kMaxEvents = 4096;
};

} // namespace httpserver
