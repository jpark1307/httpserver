#include "httpserver/Connection.hpp"

#include <spdlog/spdlog.h>

namespace httpserver {

Connection::Connection(Socket socket) : socket_(std::move(socket)) {
  spdlog::debug("Connection created fd={}", socket_.fd());
}

bool Connection::doRead() {
  char buffer[kReadBufferSize];

  ssize_t bytesRead = socket_.recv(buffer, sizeof(buffer));

  if (bytesRead == 0) {
    // EOF - client closed connection
    spdlog::debug("Connection EOF fd={}", socket_.fd());
    setState(State::Closed);
    return false;
  }

  if (bytesRead < 0) {
    // Would block - no data available yet
    return true;
  }

  // Parse the received data
  if (!request_.parse(
          std::string_view(buffer, static_cast<size_t>(bytesRead)))) {
    spdlog::warn("Failed to parse HTTP request fd={}", socket_.fd());
    setState(State::Closed);
    return false;
  }

  return true;
}

bool Connection::hasCompleteRequest() const { return request_.isComplete(); }

void Connection::setResponse(HttpResponse response) {
  writeBuffer_ = response.serialize();
  writeOffset_ = 0;
  setState(State::Writing);
}

bool Connection::doWrite() {
  if (writeOffset_ >= writeBuffer_.size()) {
    return false; // Nothing to write
  }

  const char *data = writeBuffer_.data() + writeOffset_;
  size_t remaining = writeBuffer_.size() - writeOffset_;

  ssize_t bytesSent = socket_.send(data, remaining);

  if (bytesSent < 0) {
    // Would block - try again later
    return true;
  }

  writeOffset_ += static_cast<size_t>(bytesSent);

  if (writeOffset_ >= writeBuffer_.size()) {
    spdlog::debug("Response sent fd={} bytes={}", socket_.fd(),
                  writeBuffer_.size());
    return false; // Write complete
  }

  return true; // More to write
}

bool Connection::isWriteComplete() const {
  return writeOffset_ >= writeBuffer_.size();
}

void Connection::reset() {
  request_.reset();
  writeBuffer_.clear();
  writeOffset_ = 0;
  setState(State::Reading);
}

void Connection::close() {
  setState(State::Closed);
  socket_.close();
}

} // namespace httpserver
