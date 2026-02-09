#include "httpserver/Socket.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

#include <spdlog/spdlog.h>

namespace httpserver {

Socket::Socket(int fd) noexcept : fd_(fd) {}

Socket::~Socket() { close(); }

Socket::Socket(Socket &&other) noexcept : fd_(other.fd_) { other.fd_ = -1; }

Socket &Socket::operator=(Socket &&other) noexcept {
  if (this != &other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
  }
  return *this;
}

Socket Socket::createListener(uint16_t port, int backlog) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    throw std::runtime_error("socket() failed: " +
                             std::string(strerror(errno)));
  }

  Socket sock(fd);
  sock.setReuseAddr();
  sock.setNonBlocking();

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));
  }

  if (::listen(fd, backlog) < 0) {
    throw std::runtime_error("listen() failed: " +
                             std::string(strerror(errno)));
  }

  spdlog::info("Listening socket created on port {}", port);
  return sock;
}

Socket Socket::accept() const {
  sockaddr_in clientAddr{};
  socklen_t clientLen = sizeof(clientAddr);

  int clientFd =
      ::accept(fd_, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
  if (clientFd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return Socket(-1); // No connection ready
    }
    throw std::runtime_error("accept() failed: " +
                             std::string(strerror(errno)));
  }

  Socket clientSock(clientFd);
  clientSock.setNonBlocking();

  char ipStr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
  spdlog::debug("Accepted connection from {}:{} fd={}", ipStr,
                ntohs(clientAddr.sin_port), clientFd);

  return clientSock;
}

void Socket::setNonBlocking() {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags < 0) {
    throw std::runtime_error("fcntl(F_GETFL) failed: " +
                             std::string(strerror(errno)));
  }
  if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
    throw std::runtime_error("fcntl(F_SETFL) failed: " +
                             std::string(strerror(errno)));
  }
}

void Socket::setReuseAddr() {
  int opt = 1;
  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    throw std::runtime_error("setsockopt(SO_REUSEADDR) failed: " +
                             std::string(strerror(errno)));
  }
}

ssize_t Socket::recv(char *buf, size_t len) const {
  ssize_t n = ::recv(fd_, buf, len, 0);
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return -1; // Would block, try again later
    }
    throw std::runtime_error("recv() failed: " + std::string(strerror(errno)));
  }
  return n; // 0 means EOF
}

ssize_t Socket::send(const char *buf, size_t len) const {
  ssize_t n = ::send(fd_, buf, len, 0);
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return -1; // Would block, try again later
    }
    throw std::runtime_error("send() failed: " + std::string(strerror(errno)));
  }
  return n;
}

int Socket::release() noexcept {
  int fd = fd_;
  fd_ = -1;
  return fd;
}

void Socket::close() noexcept {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

} // namespace httpserver
