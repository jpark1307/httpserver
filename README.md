# Multi-threaded Non-blocking HTTP/1.1 Server

A high-performance HTTP server written in C++17 from scratch, demonstrating deep knowledge of low-level concurrency and networking.

## Features

- **Reactor Pattern** with non-blocking I/O
- **kqueue** for kernel-level event notification (macOS)
- **Custom Thread Pool** for parallel request processing
- **Raw POSIX sockets** (no Boost.Asio, POCO, or other frameworks)
- **Thread-safe logging** with spdlog
- **Modern C++17** (smart pointers, move semantics, `std::future`)

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                   MAIN THREAD                       │
│  ┌───────────────────────────────────────────────┐  │
│  │              Event Loop (Reactor)             │  │
│  │  ┌─────────────────────────────────────────┐  │  │
│  │  │            kqueue Instance              │  │  │
│  │  │  • Monitors listening socket            │  │  │
│  │  │  • Monitors client sockets              │  │  │
│  │  └─────────────────────────────────────────┘  │  │
│  │                     │                         │  │
│  │     ┌───────────────┼───────────────┐         │  │
│  │     ▼               ▼               ▼         │  │
│  │  Accept          Read           Write         │  │
│  │  Handler        Handler        Handler        │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────┐
│                   THREAD POOL                       │
│  ┌─────────────────────────────────────────────┐    │
│  │               Task Queue                    │    │
│  └─────────────────────────────────────────────┘    │
│       │         │         │         │               │
│       ▼         ▼         ▼         ▼               │
│    Worker    Worker    Worker    Worker             │
│      1         2         3         4                │
└─────────────────────────────────────────────────────┘
```

## Requirements

- macOS (uses kqueue for event notification)
- C++17 compatible compiler (Clang or GCC)
- CMake 3.16+

## Building

```bash
# Clone or navigate to the project
cd httpserver

# Create build directory and compile
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

## Running

```bash
# From the build directory
./httpserver
```

The server starts on **http://localhost:8080** with a thread pool sized to your CPU cores.

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Welcome page with styled HTML |
| `/health` | GET | Health check (`{"status":"healthy"}`) |
| `/api/info` | GET | Server info (JSON) |

## Example Usage

```bash
# Health check
curl http://localhost:8080/health
# Output: {"status":"healthy"}

# Server info
curl http://localhost:8080/api/info
# Output: {"name": "httpserver", "version": "1.0.0", ...}

# Benchmark with wrk (if installed)
wrk -t4 -c100 -d10s http://localhost:8080/health
```

## Adding Custom Routes

Edit `src/main.cpp` to add your own routes:

```cpp
server.get("/api/custom", [](const httpserver::HttpRequest& req) {
    return httpserver::HttpResponse::json(R"({"message":"Hello!"})");
});

server.post("/api/data", [](const httpserver::HttpRequest& req) {
    // Access request body with req.body()
    return httpserver::HttpResponse::created();
});
```

## Project Structure

```
httpserver/
├── CMakeLists.txt              # Build configuration
├── include/httpserver/
│   ├── Server.hpp              # Main server class
│   ├── ThreadPool.hpp          # Thread pool with futures
│   ├── EventLoop.hpp           # kqueue event loop
│   ├── Socket.hpp              # RAII socket wrapper
│   ├── Connection.hpp          # Connection state machine
│   ├── HttpRequest.hpp         # HTTP request parser
│   └── HttpResponse.hpp        # HTTP response builder
└── src/
    ├── main.cpp                # Entry point and routes
    ├── Server.cpp
    ├── ThreadPool.cpp
    ├── EventLoop.cpp
    ├── Socket.cpp
    ├── Connection.cpp
    ├── HttpRequest.cpp
    └── HttpResponse.cpp
```

## Why Reactor + Thread Pool?

| Aspect | Thread-per-Request | Reactor + Pool |
|--------|-------------------|----------------|
| **Memory** | ~1-8MB stack per thread | Fixed N workers |
| **Max Connections** | ~1000-5000 | 10,000+ |
| **Context Switches** | O(connections)/sec | Minimal |
| **CPU Utilization** | Poor (threads block) | Excellent |

## License

MIT
