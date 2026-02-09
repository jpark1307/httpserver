cmake_minimum_required(VERSION 3.16)
project(httpserver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform detection
if(APPLE)
    add_compile_definitions(USE_KQUEUE)
elseif(UNIX)
    add_compile_definitions(USE_EPOLL)
endif()

# Fetch spdlog for logging
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Library target
add_library(httpserver_lib STATIC
    src/Socket.cpp
    src/EventLoop.cpp
    src/ThreadPool.cpp
    src/Connection.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/Server.cpp
)

target_include_directories(httpserver_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(httpserver_lib PUBLIC
    spdlog::spdlog
)

# Pthread required for threading
find_package(Threads REQUIRED)
target_link_libraries(httpserver_lib PUBLIC Threads::Threads)

# Main executable
add_executable(httpserver src/main.cpp)
target_link_libraries(httpserver PRIVATE httpserver_lib)

# Compiler warnings
target_compile_options(httpserver_lib PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)
