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

# Source files (will be added as we implement)
set(TZZERO_SOURCES
    # Core will be added later
)

# Main library (placeholder for now)
# add_library(tzzero_lib STATIC ${TZZERO_SOURCES})

# Main server executable (placeholder)
# add_executable(tzzero-http src/main.cpp)
