#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace httpserver {

/**
 * HTTP request parser and data container.
 * Parses HTTP/1.1 requests including method, path, headers, and body.
 */
class HttpRequest {
public:
  enum class Method { GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH, UNKNOWN };

  HttpRequest() = default;

  /**
   * Parse HTTP request data.
   * Can be called incrementally as data arrives.
   * @param data Raw HTTP data to parse
   * @return true if parsing succeeded (or is still in progress), false on error
   */
  bool parse(std::string_view data);

  /**
   * Check if the request is complete (all headers and body received).
   */
  bool isComplete() const { return complete_; }

  /**
   * Check if there was a parsing error.
   */
  bool hasError() const { return error_; }

  /**
   * Get the HTTP method.
   */
  Method method() const { return method_; }

  /**
   * Get the method as a string.
   */
  std::string_view methodString() const;

  /**
   * Get the request path.
   */
  std::string_view path() const { return path_; }

  /**
   * Get the HTTP version.
   */
  std::string_view version() const { return version_; }

  /**
   * Get a header value by name (case-insensitive).
   * @param name Header name
   * @return Header value or empty string if not found
   */
  std::string header(std::string_view name) const;

  /**
   * Get the request body.
   */
  std::string_view body() const { return body_; }

  /**
   * Get all headers.
   */
  const std::unordered_map<std::string, std::string> &headers() const {
    return headers_;
  }

  /**
   * Reset the request for reuse.
   */
  void reset();

private:
  bool parseRequestLine(std::string_view line);
  bool parseHeader(std::string_view line);
  Method stringToMethod(std::string_view str);

  Method method_ = Method::UNKNOWN;
  std::string path_;
  std::string version_;
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;

  std::string buffer_;
  size_t contentLength_ = 0;
  bool headersComplete_ = false;
  bool complete_ = false;
  bool error_ = false;
};

} // namespace httpserver
