#include "httpserver/HttpRequest.hpp"

#include <algorithm>
#include <cctype>

namespace httpserver {

bool HttpRequest::parse(std::string_view data) {
  if (error_ || complete_) {
    return !error_;
  }

  buffer_.append(data);

  // Parse headers if not yet complete
  if (!headersComplete_) {
    size_t headerEnd = buffer_.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
      return true; // Need more data
    }

    // Parse request line
    size_t lineEnd = buffer_.find("\r\n");
    if (lineEnd == std::string::npos || lineEnd > headerEnd) {
      error_ = true;
      return false;
    }

    if (!parseRequestLine(std::string_view(buffer_.data(), lineEnd))) {
      error_ = true;
      return false;
    }

    // Parse headers
    size_t pos = lineEnd + 2;
    while (pos < headerEnd) {
      size_t nextLine = buffer_.find("\r\n", pos);
      if (nextLine == std::string::npos || nextLine > headerEnd) {
        break;
      }
      if (nextLine > pos) {
        if (!parseHeader(
                std::string_view(buffer_.data() + pos, nextLine - pos))) {
          error_ = true;
          return false;
        }
      }
      pos = nextLine + 2;
    }

    headersComplete_ = true;

    // Check for Content-Length
    auto it = headers_.find("content-length");
    if (it != headers_.end()) {
      try {
        contentLength_ = std::stoull(it->second);
      } catch (...) {
        error_ = true;
        return false;
      }
    }

    // Remove headers from buffer, keep body
    buffer_.erase(0, headerEnd + 4);
  }

  // Check if body is complete
  if (buffer_.size() >= contentLength_) {
    body_ = buffer_.substr(0, contentLength_);
    complete_ = true;
  }

  return true;
}

bool HttpRequest::parseRequestLine(std::string_view line) {
  // Parse: METHOD PATH VERSION
  size_t methodEnd = line.find(' ');
  if (methodEnd == std::string_view::npos) {
    return false;
  }

  method_ = stringToMethod(line.substr(0, methodEnd));

  size_t pathStart = methodEnd + 1;
  size_t pathEnd = line.find(' ', pathStart);
  if (pathEnd == std::string_view::npos) {
    return false;
  }

  path_ = std::string(line.substr(pathStart, pathEnd - pathStart));
  version_ = std::string(line.substr(pathEnd + 1));

  return true;
}

bool HttpRequest::parseHeader(std::string_view line) {
  size_t colonPos = line.find(':');
  if (colonPos == std::string_view::npos) {
    return false;
  }

  std::string name(line.substr(0, colonPos));
  // Convert header name to lowercase
  std::transform(name.begin(), name.end(), name.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Skip whitespace after colon
  size_t valueStart = colonPos + 1;
  while (valueStart < line.size() &&
         std::isspace(static_cast<unsigned char>(line[valueStart]))) {
    ++valueStart;
  }

  std::string value(line.substr(valueStart));
  headers_[std::move(name)] = std::move(value);

  return true;
}

HttpRequest::Method HttpRequest::stringToMethod(std::string_view str) {
  if (str == "GET")
    return Method::GET;
  if (str == "POST")
    return Method::POST;
  if (str == "PUT")
    return Method::PUT;
  if (str == "DELETE")
    return Method::DELETE;
  if (str == "HEAD")
    return Method::HEAD;
  if (str == "OPTIONS")
    return Method::OPTIONS;
  if (str == "PATCH")
    return Method::PATCH;
  return Method::UNKNOWN;
}

std::string_view HttpRequest::methodString() const {
  switch (method_) {
  case Method::GET:
    return "GET";
  case Method::POST:
    return "POST";
  case Method::PUT:
    return "PUT";
  case Method::DELETE:
    return "DELETE";
  case Method::HEAD:
    return "HEAD";
  case Method::OPTIONS:
    return "OPTIONS";
  case Method::PATCH:
    return "PATCH";
  default:
    return "UNKNOWN";
  }
}

std::string HttpRequest::header(std::string_view name) const {
  std::string lowerName(name);
  std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  auto it = headers_.find(lowerName);
  if (it != headers_.end()) {
    return it->second;
  }
  return "";
}

void HttpRequest::reset() {
  method_ = Method::UNKNOWN;
  path_.clear();
  version_.clear();
  headers_.clear();
  body_.clear();
  buffer_.clear();
  contentLength_ = 0;
  headersComplete_ = false;
  complete_ = false;
  error_ = false;
}

} // namespace httpserver
