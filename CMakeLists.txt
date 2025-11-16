cmake_minimum_required(VERSION 3.20)
project(TZZeroHTTP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建选项
option(ENABLE_IO_URING "Enable io_uring support" OFF)
option(ENABLE_TLS "Enable TLS support" ON)
option(BUILD_BENCHMARKS "Build benchmark tools" ON)
option(BUILD_TESTS "Build tests" ON)

# 性能相关的编译器标志
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address,undefined")

# 查找依赖包
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# TLS 需要的 OpenSSL
if(ENABLE_TLS)
    find_package(OpenSSL REQUIRED)
    add_compile_definitions(ENABLE_TLS)
endif()

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# 源文件列表
set(TZZERO_SOURCES
    # 工具类
    src/utils/buffer.cpp
    src/utils/logger.cpp
    src/utils/thread_pool.cpp

    # 核心模块
    src/core/poller.cpp
    src/core/epoll_poller.cpp
    src/core/timer_queue.cpp
    src/core/event_loop.cpp

    # 网络模块
    src/net/tcp_connection.cpp
    src/net/acceptor.cpp
    src/net/event_loop_thread_pool.cpp
    src/net/tcp_server.cpp

    # HTTP 模块
    src/http/http_request.cpp
    src/http/http_response.cpp
    src/http/http_parser.cpp
    src/http/http_server.cpp
)

# 主库
add_library(tzzero_lib STATIC ${TZZERO_SOURCES})

# 链接库
target_link_libraries(tzzero_lib PRIVATE Threads::Threads)

if(ENABLE_TLS)
    target_link_libraries(tzzero_lib PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# 主服务器可执行文件
add_executable(tzzero-http src/main.cpp)
target_link_libraries(tzzero-http tzzero_lib)

# 基准测试工具
if(BUILD_BENCHMARKS)
    add_executable(tzzero-benchmark tools/benchmark.cpp)
    target_link_libraries(tzzero-benchmark tzzero_lib)
endif()

# 测试
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    include(GoogleTest)

    # 添加测试文件
    add_executable(tzzero_tests
        tests/buffer_test.cpp
        tests/logger_test.cpp
        tests/thread_pool_test.cpp
        tests/memory_pool_test.cpp
        tests/http_parser_test.cpp
        tests/http_request_test.cpp
        tests/http_response_test.cpp
    )

    target_link_libraries(tzzero_tests
        tzzero_lib
        GTest::gtest_main
        GTest::gmock_main
    )

    gtest_discover_tests(tzzero_tests)
endif()






