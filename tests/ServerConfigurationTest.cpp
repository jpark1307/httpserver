#include "httpserver/Server.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

int main() {
  static_assert(
      std::is_constructible_v<httpserver::Server, uint16_t, std::size_t>,
      "The existing two-argument Server constructor must remain supported");

  try {
    httpserver::Server server(0, 1, 0);
  } catch (const std::invalid_argument &) {
    return 0;
  }

  return 1;
}
