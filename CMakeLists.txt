cmake_minimum_required(VERSION 3.20)
project(TZZeroHTTP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(ENABLE_IO_URING "Enable io_uring support" OFF)
option(ENABLE_TLS "Enable TLS support" ON)
option(BUILD_BENCHMARKS "Build benchmark tools" ON)
option(BUILD_TESTS "Build tests" ON)

# Compiler flags for performance
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address,undefined")

# Find packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# OpenSSL for TLS
if(ENABLE_TLS)
    find_package(OpenSSL REQUIRED)
    add_compile_definitions(ENABLE_TLS)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# Source files
set(TZZERO_SOURCES
    # Utils
    src/utils/buffer.cpp
    src/utils/logger.cpp
    
    # Core
    src/core/poller.cpp
    
    # Net (to be added)
    # src/net/tcp_connection.cpp
    # src/net/tcp_server.cpp
    
    # HTTP (to be added)
    # src/http/http_server.cpp
    # src/http/http_request.cpp
    # src/http/http_response.cpp
)

# Main library
add_library(tzzero_lib STATIC ${TZZERO_SOURCES})

# Link libraries
target_link_libraries(tzzero_lib PRIVATE Threads::Threads)

if(ENABLE_TLS)
    target_link_libraries(tzzero_lib PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# Main server executable (placeholder for now)
# add_executable(tzzero-http src/main.cpp)
# target_link_libraries(tzzero-http tzzero_lib)
