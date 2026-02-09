#include "httpserver/EventLoop.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/event.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace httpserver {

EventLoop::EventLoop() {
  kqueueFd_ = kqueue();
  if (kqueueFd_ < 0) {
    throw std::runtime_error("kqueue() failed: " +
                             std::string(strerror(errno)));
  }
  spdlog::debug("Created kqueue fd={}", kqueueFd_);
}

EventLoop::~EventLoop() {
  if (kqueueFd_ >= 0) {
    ::close(kqueueFd_);
  }
}

void EventLoop::addFd(int fd, EventType type, void *userData) {
  struct kevent changes[2];
  int numChanges = 0;

  if (type == EventType::Read || type == EventType::ReadWrite) {
    EV_SET(&changes[numChanges++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
           userData);
  }
  if (type == EventType::Write || type == EventType::ReadWrite) {
    EV_SET(&changes[numChanges++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0,
           userData);
  }

  if (kevent(kqueueFd_, changes, numChanges, nullptr, 0, nullptr) < 0) {
    throw std::runtime_error("kevent(add) failed: " +
                             std::string(strerror(errno)));
  }
  spdlog::trace("Added fd={} to kqueue", fd);
}

void EventLoop::modifyFd(int fd, EventType type, void *userData) {
  // In kqueue, we need to delete then add with new settings
  struct kevent changes[4];
  int numChanges = 0;

  // First disable the read filter (write filter may not exist yet)
  EV_SET(&changes[numChanges++], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);

  // Re-add desired filters
  if (type == EventType::Read || type == EventType::ReadWrite) {
    EV_SET(&changes[numChanges++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
           userData);
  }
  if (type == EventType::Write || type == EventType::ReadWrite) {
    EV_SET(&changes[numChanges++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0,
           userData);
  }

  // Apply changes
  int result = kevent(kqueueFd_, changes, numChanges, nullptr, 0, nullptr);
  if (result < 0 && errno != ENOENT) { // ENOENT is OK (filter didn't exist)
    spdlog::error("kevent(modify) failed for fd={}: {}", fd, strerror(errno));
  } else {
    spdlog::debug("Modified fd={} in kqueue, type={}", fd,
                  static_cast<int>(type));
  }
}

void EventLoop::removeFd(int fd) {
  struct kevent changes[2];
  EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
  EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);

  // Ignore errors (filters may not exist)
  kevent(kqueueFd_, changes, 2, nullptr, 0, nullptr);
  spdlog::trace("Removed fd={} from kqueue", fd);
}

std::vector<Event> EventLoop::wait(int timeoutMs) {
  struct kevent events[kMaxEvents];

  struct timespec timeout;
  struct timespec *timeoutPtr = nullptr;

  if (timeoutMs >= 0) {
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_nsec = (timeoutMs % 1000) * 1000000L;
    timeoutPtr = &timeout;
  }

  int numEvents = kevent(kqueueFd_, nullptr, 0, events, kMaxEvents, timeoutPtr);

  if (numEvents < 0) {
    if (errno == EINTR) {
      return {}; // Interrupted, return empty
    }
    throw std::runtime_error("kevent(wait) failed: " +
                             std::string(strerror(errno)));
  }

  std::vector<Event> result;
  result.reserve(static_cast<size_t>(numEvents));

  for (int i = 0; i < numEvents; ++i) {
    Event evt;
    evt.fd = static_cast<int>(events[i].ident);
    evt.userData = events[i].udata;
    evt.isError = (events[i].flags & EV_ERROR) != 0;
    evt.isEof = (events[i].flags & EV_EOF) != 0;

    if (events[i].filter == EVFILT_READ) {
      evt.type = EventType::Read;
    } else if (events[i].filter == EVFILT_WRITE) {
      evt.type = EventType::Write;
    } else {
      continue; // Skip unknown filter types
    }

    result.push_back(evt);
  }

  return result;
}

void EventLoop::stop() { running_.store(false, std::memory_order_relaxed); }

} // namespace httpserver
