#include "httpserver/HttpResponse.hpp"

#include <sstream>

namespace httpserver {

HttpResponse &HttpResponse::status(int code, std::string_view reason) {
  statusCode_ = code;
  if (reason.empty()) {
    statusReason_ = std::string(defaultReasonPhrase(code));
  } else {
    statusReason_ = std::string(reason);
  }
  return *this;
}

HttpResponse &HttpResponse::header(std::string_view name,
                                   std::string_view value) {
  headers_[std::string(name)] = std::string(value);
  return *this;
}

HttpResponse &HttpResponse::body(std::string content) {
  body_ = std::move(content);
  headers_["Content-Length"] = std::to_string(body_.size());
  return *this;
}

HttpResponse &HttpResponse::contentType(std::string_view type) {
  headers_["Content-Type"] = std::string(type);
  return *this;
}

std::string HttpResponse::serialize() const {
  std::ostringstream ss;

  // Status line
  ss << "HTTP/1.1 " << statusCode_ << " " << statusReason_ << "\r\n";

  // Headers
  for (const auto &[name, value] : headers_) {
    ss << name << ": " << value << "\r\n";
  }

  // End of headers
  ss << "\r\n";

  // Body
  ss << body_;

  return ss.str();
}

HttpResponse HttpResponse::ok() { return HttpResponse().status(200); }

HttpResponse HttpResponse::created() { return HttpResponse().status(201); }

HttpResponse HttpResponse::noContent() { return HttpResponse().status(204); }

HttpResponse HttpResponse::badRequest() {
  return HttpResponse()
      .status(400)
      .contentType("text/plain")
      .body("Bad Request");
}

HttpResponse HttpResponse::notFound() {
  return HttpResponse().status(404).contentType("text/plain").body("Not Found");
}

HttpResponse HttpResponse::methodNotAllowed() {
  return HttpResponse()
      .status(405)
      .contentType("text/plain")
      .body("Method Not Allowed");
}

HttpResponse HttpResponse::internalError() {
  return HttpResponse()
      .status(500)
      .contentType("text/plain")
      .body("Internal Server Error");
}

HttpResponse HttpResponse::html(std::string content) {
  return HttpResponse()
      .status(200)
      .contentType("text/html; charset=utf-8")
      .body(std::move(content));
}

HttpResponse HttpResponse::json(std::string content) {
  return HttpResponse()
      .status(200)
      .contentType("application/json")
      .body(std::move(content));
}

HttpResponse HttpResponse::text(std::string content) {
  return HttpResponse()
      .status(200)
      .contentType("text/plain")
      .body(std::move(content));
}

std::string_view HttpResponse::defaultReasonPhrase(int code) {
  switch (code) {
  case 100:
    return "Continue";
  case 101:
    return "Switching Protocols";
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 202:
    return "Accepted";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 304:
    return "Not Modified";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 408:
    return "Request Timeout";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  default:
    return "Unknown";
  }
}

} // namespace httpserver
