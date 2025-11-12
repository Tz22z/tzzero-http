# TZZero-HTTP

基于 C++ 的事件驱动 HTTP 服务器，Reactor 模式 + epoll，少量线程处理大量并发。

## 简介

这个项目实现了一个高性能的 HTTP/1.1 服务器框架。采用 one loop per thread 的线程模型，通过 epoll 做 I/O 多路复用，非阻塞 I/O 配合事件驱动，少量线程就能同时处理上万个连接。

主线程作为 MainReactor 接受新连接，Round Robin 分发给工作线程。工作线程作为 SubReactor 负责具体的 I/O 和业务处理。

## 核心功能

**HTTP/1.1 协议**

支持常见的 GET、POST、PUT、DELETE 方法。流式解析请求，状态机保证正确性。支持 Keep-Alive 持久连接，一个 TCP 连接可以复用发送多个请求。

路由支持精确匹配和正则表达式。可以为不同方法和路径注册处理函数。

优雅关闭机制，不再接受新连接，但会等现有请求处理完再退出。

**内置组件**

Logger - 异步日志系统，双缓冲机制，前端线程写缓冲区，后端线程刷盘。支持日志级别和滚动文件。

Buffer - 网络数据缓冲区，动态扩容，readv/writev 减少系统调用。参考 muduo 设计。

ThreadPool - 通用线程池，用来执行耗时任务，避免阻塞事件循环。

MemoryPool - 固定大小对象的内存池，预分配内存，空闲链表管理。减少 malloc/free 开销，对小对象性能提升明显。

TimerQueue - 定时器队列，基于 timerfd + epoll，小顶堆管理定时任务。支持一次性和周期性定时器。

## 编译运行

需要 Linux 系统，C++17 编译器（GCC 9+ 或 Clang 10+），CMake 3.15+。

无外部依赖，标准库就够。测试用 Google Test，会自动下载。

mkdir build && cd build
cmake ..
make

## 性能

在 4 核机器上测试，简单静态响应 QPS 10 万+，P99 延迟 10ms 以内。能同时处理上万个并发连接。

用 wrk 或 ab 可以跑压测。实际性能和业务逻辑有关，耗时操作建议放线程池执行。

## 测试

82 个测试用例，覆盖 Buffer、Logger、ThreadPool、MemoryPool、HTTP 解析。

## 示例

examples 目录下有几个示例程序，可以直接编译运行。

## TODO

HTTP/2 支持（多路复用、头部压缩）
TLS/SSL 加密传输
WebSocket 完整实现
中间件系统（参考 Express）
io_uring 替换 epoll

## 参考

设计参考了 muduo 网络库，学到了很多网络编程和 C++ 实践的知识。

## License

MIT
