#include "httpserver/Server.hpp"

#include <spdlog/spdlog.h>

namespace httpserver {

Server::Server(uint16_t port, size_t threadPoolSize)
    : listenSocket_(Socket::createListener(port)), eventLoop_(),
      threadPool_(threadPoolSize), port_(port) {

  // Register listening socket for accept events
  eventLoop_.addFd(listenSocket_.fd(), EventType::Read, nullptr);

  spdlog::info("Server initialized on port {} with {} worker threads", port,
               threadPool_.numWorkers());
}

Server::~Server() { stop(); }

void Server::route(HttpRequest::Method method, std::string path,
                   RequestHandler handler) {
  routes_[{method, std::move(path)}] = std::move(handler);
}

void Server::get(std::string path, RequestHandler handler) {
  route(HttpRequest::Method::GET, std::move(path), std::move(handler));
}

void Server::post(std::string path, RequestHandler handler) {
  route(HttpRequest::Method::POST, std::move(path), std::move(handler));
}

void Server::put(std::string path, RequestHandler handler) {
  route(HttpRequest::Method::PUT, std::move(path), std::move(handler));
}

void Server::del(std::string path, RequestHandler handler) {
  route(HttpRequest::Method::DELETE, std::move(path), std::move(handler));
}

void Server::run() {
  spdlog::info("Server starting on port {}", port_);

  while (eventLoop_.isRunning()) {
    // Process any pending writes from worker threads
    processPendingWrites();

    auto events =
        eventLoop_.wait(10); // 10ms timeout to check pending writes frequently

    for (const auto &event : events) {
      if (event.fd == listenSocket_.fd()) {
        // New connection
        handleAccept();
      } else {
        // Client socket event
        std::shared_ptr<Connection> conn;
        {
          std::lock_guard<std::mutex> lock(connectionsMutex_);
          auto it = connections_.find(event.fd);
          if (it == connections_.end()) {
            continue; // Connection already closed
          }
          conn = it->second;
        }

        if (event.isError || event.isEof) {
          closeConnection(event.fd);
          continue;
        }

        if (event.type == EventType::Read) {
          handleRead(conn);
        } else if (event.type == EventType::Write) {
          handleWrite(conn);
        }
      }
    }
  }

  spdlog::info("Server stopped");
}

void Server::stop() {
  eventLoop_.stop();
  threadPool_.shutdown();

  // Close all connections
  std::lock_guard<std::mutex> lock(connectionsMutex_);
  for (auto &[fd, conn] : connections_) {
    conn->close();
  }
  connections_.clear();
}

void Server::handleAccept() {
  while (true) {
    Socket clientSocket = listenSocket_.accept();
    if (!clientSocket.valid()) {
      break; // No more pending connections
    }

    int fd = clientSocket.fd();
    auto conn = std::make_shared<Connection>(std::move(clientSocket));

    {
      std::lock_guard<std::mutex> lock(connectionsMutex_);
      connections_[fd] = conn;
    }

    // Register for read events
    eventLoop_.addFd(fd, EventType::Read, conn.get());
    spdlog::debug("Accepted new connection fd={}", fd);
  }
}

void Server::handleRead(std::shared_ptr<Connection> conn) {
  if (!conn->doRead()) {
    closeConnection(conn->fd());
    return;
  }

  if (conn->hasCompleteRequest()) {
    conn->setState(Connection::State::Processing);

    spdlog::debug("Request complete fd={}, submitting to thread pool",
                  conn->fd());

    // Submit request processing to thread pool
    auto weakConn = std::weak_ptr<Connection>(conn);
    threadPool_.submit([this, weakConn]() {
      if (auto conn = weakConn.lock()) {
        processRequest(conn);
      }
    });
  }
}

void Server::handleWrite(std::shared_ptr<Connection> conn) {
  if (!conn->doWrite() || conn->isWriteComplete()) {
    // Response sent - close connection (no keep-alive for simplicity)
    spdlog::debug("Write complete fd={}, closing connection", conn->fd());
    closeConnection(conn->fd());
  }
}

void Server::processRequest(std::shared_ptr<Connection> conn) {
  const auto &request = conn->request();

  spdlog::debug("Processing {} {} fd={}", request.methodString(),
                request.path(), conn->fd());

  // Route the request
  HttpResponse response = routeRequest(request);

  // Add standard headers
  response.header("Server", "httpserver/1.0");
  response.header("Connection", "close");

  // Set response and switch to writing state
  conn->setResponse(std::move(response));

  // Queue for write event registration (will be processed by main thread)
  {
    std::lock_guard<std::mutex> lock(pendingWritesMutex_);
    pendingWrites_.push_back(conn);
  }

  spdlog::debug("Response queued for fd={}", conn->fd());
}

void Server::processPendingWrites() {
  std::vector<std::shared_ptr<Connection>> toProcess;

  {
    std::lock_guard<std::mutex> lock(pendingWritesMutex_);
    toProcess.swap(pendingWrites_);
  }

  for (auto &conn : toProcess) {
    if (!conn->isClosed()) {
      eventLoop_.modifyFd(conn->fd(), EventType::Write, conn.get());
      spdlog::debug("Enabled write events for fd={}", conn->fd());
    }
  }
}

HttpResponse Server::routeRequest(const HttpRequest &request) {
  auto it = routes_.find({request.method(), std::string(request.path())});

  if (it != routes_.end()) {
    try {
      return it->second(request);
    } catch (const std::exception &e) {
      spdlog::error("Handler exception: {}", e.what());
      return HttpResponse::internalError();
    }
  }

  // Check if path exists with different method
  for (const auto &[key, handler] : routes_) {
    if (key.second == request.path()) {
      return HttpResponse::methodNotAllowed();
    }
  }

  return HttpResponse::notFound();
}

void Server::closeConnection(int fd) {
  eventLoop_.removeFd(fd);

  std::lock_guard<std::mutex> lock(connectionsMutex_);
  auto it = connections_.find(fd);
  if (it != connections_.end()) {
    it->second->close();
    connections_.erase(it);
    spdlog::debug("Closed connection fd={}", fd);
  }
}

} // namespace httpserver
