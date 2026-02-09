#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace httpserver {

/**
 * RAII wrapper for socket file descriptors.
 * Move-only semantics prevent accidental fd leaks.
 */
class Socket {
public:
    explicit Socket(int fd = -1) noexcept;
    ~Socket();

    // Move-only (no copies)
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    /**
     * Create a listening socket bound to the specified port.
     * @param port The port to listen on
     * @param backlog Maximum pending connections queue size
     * @return A Socket ready to accept connections
     * @throws std::runtime_error on failure
     */
    static Socket createListener(uint16_t port, int backlog = 128);

    /**
     * Accept a new connection from this listening socket.
     * @return A new Socket for the accepted connection
     * @throws std::runtime_error on failure
     */
    Socket accept() const;

    /**
     * Set socket to non-blocking mode.
     * @throws std::runtime_error on failure
     */
    void setNonBlocking();

    /**
     * Enable SO_REUSEADDR option.
     * @throws std::runtime_error on failure
     */
    void setReuseAddr();

    /**
     * Receive data from socket.
     * @param buf Buffer to receive into
     * @param len Maximum bytes to receive
     * @return Number of bytes received, 0 on EOF, -1 on EAGAIN/EWOULDBLOCK
     */
    ssize_t recv(char* buf, size_t len) const;

    /**
     * Send data through socket.
     * @param buf Data to send
     * @param len Number of bytes to send
     * @return Number of bytes sent, -1 on EAGAIN/EWOULDBLOCK
     */
    ssize_t send(const char* buf, size_t len) const;

    /**
     * Get the underlying file descriptor.
     */
    int fd() const noexcept { return fd_; }

    /**
     * Check if socket is valid.
     */
    bool valid() const noexcept { return fd_ >= 0; }

    /**
     * Release ownership of the file descriptor.
     * @return The file descriptor (caller must close it)
     */
    int release() noexcept;

    /**
     * Close the socket.
     */
    void close() noexcept;

private:
    int fd_;
};

} // namespace httpserver
