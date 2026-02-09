#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace httpserver {

/**
 * HTTP response builder.
 * Supports fluent interface for building HTTP/1.1 responses.
 */
class HttpResponse {
public:
  HttpResponse() = default;

  /**
   * Set the status code and optional reason phrase.
   * @param code HTTP status code
   * @param reason Optional reason phrase (default based on code)
   * @return Reference to this for chaining
   */
  HttpResponse &status(int code, std::string_view reason = "");

  /**
   * Add a header to the response.
   * @param name Header name
   * @param value Header value
   * @return Reference to this for chaining
   */
  HttpResponse &header(std::string_view name, std::string_view value);

  /**
   * Set the response body.
   * Automatically sets Content-Length header.
   * @param content Body content
   * @return Reference to this for chaining
   */
  HttpResponse &body(std::string content);

  /**
   * Set the Content-Type header.
   * @param type MIME type
   * @return Reference to this for chaining
   */
  HttpResponse &contentType(std::string_view type);

  /**
   * Serialize the response to a string for sending.
   * @return Complete HTTP response as string
   */
  std::string serialize() const;

  // Convenience factory methods
  static HttpResponse ok();
  static HttpResponse created();
  static HttpResponse noContent();
  static HttpResponse badRequest();
  static HttpResponse notFound();
  static HttpResponse methodNotAllowed();
  static HttpResponse internalError();

  /**
   * Create an HTML response with the given content.
   */
  static HttpResponse html(std::string content);

  /**
   * Create a JSON response with the given content.
   */
  static HttpResponse json(std::string content);

  /**
   * Create a plain text response.
   */
  static HttpResponse text(std::string content);

private:
  static std::string_view defaultReasonPhrase(int code);

  int statusCode_ = 200;
  std::string statusReason_ = "OK";
  std::unordered_map<std::string, std::string> headers_;
  std::string body_;
};

} // namespace httpserver
