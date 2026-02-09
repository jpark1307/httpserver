#pragma once

#include "httpserver/HttpRequest.hpp"
#include "httpserver/HttpResponse.hpp"
#include "httpserver/Socket.hpp"

#include <atomic>
#include <memory>
#include <string>

namespace httpserver {

/**
 * Represents a single client connection with state machine.
 * Manages the lifecycle of reading request -> processing -> writing response.
 */
class Connection : public std::enable_shared_from_this<Connection> {
public:
  enum class State {
    Reading,    // Waiting for complete request
    Processing, // Request being processed by worker thread
    Writing,    // Sending response
    Closed      // Connection closed
  };

  explicit Connection(Socket socket);
  ~Connection() = default;

  // Non-copyable
  Connection(const Connection &) = delete;
  Connection &operator=(const Connection &) = delete;

  /**
   * Read data from socket into internal buffer.
   * @return true if data was read, false if connection should be closed
   */
  bool doRead();

  /**
   * Check if a complete request has been received.
   */
  bool hasCompleteRequest() const;

  /**
   * Get the parsed HTTP request.
   */
  HttpRequest &request() { return request_; }
  const HttpRequest &request() const { return request_; }

  /**
   * Set the response to send.
   * Also transitions state to Writing.
   */
  void setResponse(HttpResponse response);

  /**
   * Write pending response data to socket.
   * @return true if more data to write, false if complete or error
   */
  bool doWrite();

  /**
   * Check if all response data has been sent.
   */
  bool isWriteComplete() const;

  /**
   * Get the current connection state.
   */
  State state() const { return state_.load(std::memory_order_acquire); }

  /**
   * Set the connection state.
   */
  void setState(State state) { state_.store(state, std::memory_order_release); }

  /**
   * Get the underlying file descriptor.
   */
  int fd() const { return socket_.fd(); }

  /**
   * Reset the connection for keep-alive reuse.
   */
  void reset();

  /**
   * Close the connection.
   */
  void close();

  /**
   * Check if the connection is closed.
   */
  bool isClosed() const {
    return state_.load(std::memory_order_acquire) == State::Closed;
  }

private:
  Socket socket_;
  std::atomic<State> state_{State::Reading};

  HttpRequest request_;
  std::string writeBuffer_;
  size_t writeOffset_ = 0;

  static constexpr size_t kReadBufferSize = 4096;
};

} // namespace httpserver
