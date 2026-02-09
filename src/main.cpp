#include "httpserver/Server.hpp"

#include <spdlog/spdlog.h>

#include <atomic>
#include <csignal>

namespace {
std::atomic<bool> g_running{true};

void signalHandler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    spdlog::info("Received signal {}, shutting down...", signal);
    g_running = false;
  }
}
} // namespace

int main() {
  // Configure logging
  spdlog::set_level(spdlog::level::debug);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");

  // Setup signal handlers
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  try {
    // Create server on port 8080
    httpserver::Server server(8080);

    // Register routes
    server.get("/", [](const httpserver::HttpRequest &) {
      return httpserver::HttpResponse::html(R"(
<!DOCTYPE html>
<html>
<head>
    <title>HTTP Server</title>
    <style>
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            max-width: 800px; 
            margin: 50px auto; 
            padding: 20px;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #eee;
            min-height: 100vh;
        }
        h1 { 
            color: #7c3aed;
            border-bottom: 2px solid #7c3aed;
            padding-bottom: 10px;
        }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 12px;
            padding: 20px;
            margin: 20px 0;
            backdrop-filter: blur(10px);
        }
        code {
            background: rgba(124, 58, 237, 0.2);
            padding: 2px 6px;
            border-radius: 4px;
            font-family: 'Fira Code', monospace;
        }
        .badge {
            display: inline-block;
            background: #10b981;
            color: white;
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 0.85em;
            margin: 5px 5px 5px 0;
        }
    </style>
</head>
<body>
    <h1>🚀 Multi-threaded HTTP Server</h1>
    <div class="card">
        <h2>Features</h2>
        <p><span class="badge">Non-blocking I/O</span>
           <span class="badge">kqueue</span>
           <span class="badge">Thread Pool</span>
           <span class="badge">C++17</span></p>
        <p>This server uses the <strong>Reactor pattern</strong> with <code>kqueue</code> for 
           efficient event notification and a thread pool for parallel request processing.</p>
    </div>
    <div class="card">
        <h2>Endpoints</h2>
        <ul>
            <li><code>GET /</code> - This page</li>
            <li><code>GET /health</code> - Health check</li>
            <li><code>GET /api/info</code> - Server info (JSON)</li>
        </ul>
    </div>
</body>
</html>
            )");
    });

    server.get("/health", [](const httpserver::HttpRequest &) {
      return httpserver::HttpResponse::json(R"({"status":"healthy"})");
    });

    server.get("/api/info", [](const httpserver::HttpRequest &) {
      return httpserver::HttpResponse::json(R"({
    "name": "httpserver",
    "version": "1.0.0",
    "architecture": "Reactor + Thread Pool",
    "event_system": "kqueue",
    "language": "C++17"
})");
    });

    // Run the server
    spdlog::info("Starting HTTP server on http://localhost:8080");
    spdlog::info("Press Ctrl+C to stop");

    // Run in a thread so we can check g_running
    std::thread serverThread([&server]() { server.run(); });

    // Wait for shutdown signal
    while (g_running && server.isRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server.stop();
    serverThread.join();

  } catch (const std::exception &e) {
    spdlog::error("Fatal error: {}", e.what());
    return 1;
  }

  return 0;
}
