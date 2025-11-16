# TZZero-HTTP

基于C++和事件驱动模型的 HTTP 服务器框架

## 主要特性

核心功能基于 epoll 的事件驱动模型，用 one loop per thread 的方式处理并发连接。支持 HTTP/1.1 协议，包括持久连接。服务器关闭时会等待现有连接处理完成。

内置了一些常用组件：
Logger 做日志记录，线程安全，可以输出到文件
Buffer 管理网络数据，支持 readv/writev
ThreadPool 通用线程池
MemoryPool 内存池模板
TimerQueue 定时器

## 编译

需要支持 C++20 的编译器，GCC 11 或 Clang 14 以上。CMake 要 3.20 以上。只能在 Linux 上跑，因为用了 epoll。

```bash
git clone https://github.com/yourusername/tzzero-http.git
cd tzzero-http
mkdir build && cd build
cmake ..
make -j4
```

可以运行测试：
```bash
./tzzero_tests
```

如果要编译优化版本：
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

## 使用方法

最简单的例子：

```cpp
#include "tzzero/http/http_server.h"
#include "tzzero/http/http_request.h"
#include "tzzero/http/http_response.h"

using namespace tzzero::http;

int main() {
    HttpServer server("0.0.0.0", 8080);

    server.route("/", [](const HttpRequest& req, HttpResponse& resp) {
        resp.set_status_code(HttpStatusCode::OK);
        resp.set_html_content_type();
        resp.set_body("<h1>Hello, World!</h1>");
    });

    server.start();
    return 0;
}
```

编译之后直接运行：
```bash
./tzzero-http
```

可以指定一些参数：
```bash
./tzzero-http --port 8080 --threads 4
./tzzero-http --log-level DEBUG
./tzzero-http --help
```

## 示例代码

examples 目录下有几个例子：

hello_world.cpp 最基础的服务器
rest_api.cpp 实现了一个简单的 RESTful API
static_files.cpp 静态文件服务器
websocket_echo.cpp WebSocket 的基础框架

具体用法看 examples/README.md



## 性能

可以用 wrk 或者 ab 来测试：

```bash
wrk -t4 -c100 -d30s http://localhost:3000/
ab -n 100000 -c 1000 http://localhost:3000/
```

在 4 核 CPU、8GB 内存的机器上，简单的静态响应能跑到 10 万以上 QPS，P99 延迟在 10ms 以内。能同时处理上万个连接。

## 测试

```bash
cd build
./tzzero_tests
```

可以跑特定的测试：
```bash
./tzzero_tests --gtest_filter=BufferTest.*
```

目前有 82 个测试用例，覆盖了 Buffer、Logger、ThreadPool、MemoryPool 和 HTTP 协议解析。

## 文档

docs 目录下有一些文档：

architecture.md 讲架构设计
api.md 是 API 参考
development.md 开发指南
performance.md 性能优化的一些建议

## 后续计划

目前实现了基本的 HTTP/1.1 服务器，事件驱动架构，线程池和内存池。

还想加的功能：
HTTP/2 支持
TLS/SSL
完整的 WebSocket 实现
中间件系统
会话管理
静态文件缓存
io_uring 支持

## 参考

这个项目参考了 muduo 网络库的设计。测试用的是 Google Test。








