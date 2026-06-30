#pragma once

#include "httpserver/Connection.hpp"
#include "httpserver/EventLoop.hpp"
#include "httpserver/HttpRequest.hpp"
#include "httpserver/HttpResponse.hpp"
#include "httpserver/Socket.hpp"
#include "httpserver/ThreadPool.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace httpserver {

/**
 * Main HTTP server class.
 * Uses Reactor pattern with kqueue for event notification
 * and a thread pool for request processing.
 */
class Server {
public:
  using RequestHandler = std::function<HttpResponse(const HttpRequest &)>;

  /**
   * Create a server listening on the specified port.
   * @param port TCP port to listen on
   * @param threadPoolSize Number of worker threads (0 = auto-detect)
   * @param listenBacklog Maximum pending connections queue size
   * @throws std::invalid_argument if listenBacklog is not positive
   */
  explicit Server(
      uint16_t port, size_t threadPoolSize = 0,
      int listenBacklog = Socket::kDefaultListenBacklog);
  ~Server();

  // Non-copyable, non-movable
  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;
  Server(Server &&) = delete;
  Server &operator=(Server &&) = delete;

  /**
   * Register a route handler.
   * @param method HTTP method to match
   * @param path URL path to match (exact match)
   * @param handler Function to handle matching requests
   */
  void route(HttpRequest::Method method, std::string path,
             RequestHandler handler);

  /**
   * Convenience methods for common HTTP methods.
   */
  void get(std::string path, RequestHandler handler);
  void post(std::string path, RequestHandler handler);
  void put(std::string path, RequestHandler handler);
  void del(std::string path, RequestHandler handler);

  /**
   * Start the server event loop (blocking).
   * Returns when stop() is called.
   */
  void run();

  /**
   * Stop the server gracefully.
   */
  void stop();

  /**
   * Check if the server is running.
   */
  bool isRunning() const { return eventLoop_.isRunning(); }

private:
  void handleAccept();
  void handleRead(std::shared_ptr<Connection> conn);
  void handleWrite(std::shared_ptr<Connection> conn);
  void processRequest(std::shared_ptr<Connection> conn);
  void closeConnection(int fd);
  void processPendingWrites();

  HttpResponse routeRequest(const HttpRequest &request);

  Socket listenSocket_;
  EventLoop eventLoop_;
  ThreadPool threadPool_;

  std::unordered_map<int, std::shared_ptr<Connection>> connections_;
  std::mutex connectionsMutex_;

  // Thread-safe queue for connections ready to write
  std::vector<std::shared_ptr<Connection>> pendingWrites_;
  std::mutex pendingWritesMutex_;

  // Routing table: (method, path) -> handler
  using RouteKey = std::pair<HttpRequest::Method, std::string>;
  std::map<RouteKey, RequestHandler> routes_;

  uint16_t port_;
};

} // namespace httpserver
