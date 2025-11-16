# 架构设计

## 整体架构

TZZero-HTTP 用的是 Reactor 模式，事件驱动的网络服务器。代码分了几层：

HTTP 层负责解析 HTTP 请求，生成响应，管理路由。
网络层管理 TCP 连接，处理连接的建立和关闭。
事件层是核心，运行事件循环，处理 I/O 事件。
工具层提供一些基础组件，比如 Buffer、Logger、线程池。

## Reactor 模式

Reactor 是网络服务器常用的模式。核心思想是用一个循环等待 I/O 事件，事件来了就调用对应的处理函数。

EventLoop 就是这个循环，不停地调用 epoll_wait 等待事件。
EpollPoller 封装了 epoll，管理文件描述符。
TcpConnection 是事件处理器，每个连接对应一个。
Acceptor 专门处理新连接的建立。

## One Loop Per Thread

每个线程跑一个独立的 EventLoop。主线程的 EventLoop 负责接受新连接，然后把连接分配给工作线程。每个工作线程有自己的 EventLoop，处理分配给它的连接。

这样做的好处是简化了线程同步。每个连接只在一个线程里处理，不需要加锁。而且对 CPU 缓存友好，连接的数据都在同一个线程里访问。

## 核心组件

### EventLoop

事件循环是整个架构的核心。它做三件事：

调用 Poller::poll() 等待 I/O 事件。这个调用会阻塞，直到有事件发生或者超时。
处理活跃的事件。poll() 返回之后，遍历所有活跃的 Channel，调用它们的回调函数。
执行一些异步任务。其他线程可以往 EventLoop 里投递任务，在下一轮循环时执行。

还有定时器的支持。可以在 EventLoop 里添加定时任务，到时间了就会被触发。

### EpollPoller

封装了 Linux 的 epoll。epoll 是高效的 I/O 多路复用机制，可以同时监听很多文件描述符。

Poller 维护了一个 epoll fd 和一个 Channel 映射表。每个 Channel 对应一个文件描述符。当有事件发生时，epoll_wait 返回，Poller 找到对应的 Channel，加入活跃列表。

### TcpServer

管理整个服务器。它有一个 Acceptor 用来接受新连接，有一个 EventLoopThreadPool 用来管理工作线程。

新连接来了，Acceptor 创建 socket，然后 TcpServer 创建一个 TcpConnection，分配给某个工作线程的 EventLoop。

### TcpConnection

代表一个 TCP 连接。它管理这个连接的所有状态：socket fd、输入输出缓冲区、各种回调函数。

读事件来了，TcpConnection 从 socket 读数据到 input buffer，然后调用 message callback。
写事件来了，TcpConnection 把 output buffer 的数据写到 socket。
如果一次写不完，就关注可写事件，等 socket 可写了再继续写。

### Buffer

网络数据的缓冲区。内部是一个 vector<char>，维护了读写指针。

Buffer 会自动扩容。如果空间不够，就 resize vector。还有个优化是如果前面有空闲空间，会先移动数据，避免不必要的扩容。

支持 readv 和 writev。readv 可以一次读到多个缓冲区，减少系统调用。writev 可以一次写多个缓冲区。

### HttpServer

基于 TcpServer 实现的 HTTP 服务器。它维护了一个路由表，根据请求路径调用对应的处理函数。

每个连接都有一个 HttpParser，用来解析 HTTP 请求。Parser 是增量式的，数据来一点解析一点，不需要等完整的请求。

解析完成后，HttpServer 查路由表，找到处理函数，调用它。处理函数填充 HttpResponse，然后 HttpServer 把响应序列化，发送出去。

## 数据流

客户端发起连接，Acceptor 收到事件，创建新的 socket。
TcpServer 创建 TcpConnection，分配给某个 EventLoop。
客户端发送数据，TcpConnection 收到读事件，读取数据到 Buffer。
HttpParser 解析 Buffer 里的数据，构造 HttpRequest。
找到路由，调用用户的回调函数，得到 HttpResponse。
把 Response 序列化到 output buffer，写到 socket。
如果一次写不完，关注可写事件，下次继续写。

## 线程模型

主线程跑一个 EventLoop，只负责接受新连接。
有 N 个工作线程，每个都跑一个 EventLoop，处理连接上的 I/O。
新连接用 round-robin 的方式分配给工作线程。

这是经典的 multiple reactors 模式。主线程是 acceptor reactor，工作线程是 worker reactor。

## 线程安全

大部分情况下不需要锁。因为每个连接都在固定的线程里处理。

如果需要跨线程操作，用 run_in_loop()。它会把任务投递到目标线程的 EventLoop，在那个线程里执行。

少数几个地方需要锁：
Logger 是全局的，多线程写日志要加锁。
MemoryPool 可能被多个线程用，分配和释放要加锁。
用户自己的共享数据，需要自己保证线程安全。

## 性能优化

用 epoll 的 level trigger 模式。虽然 edge trigger 更高效，但 level trigger 更容易用对。

Buffer 用 readv 系统调用。可以一次读到多个缓冲区，避免多次调用 read。

连接分配用 round-robin，自动负载均衡。不需要手动管理线程间的连接分布。

对象复用。TcpConnection 和 Buffer 可以复用，减少内存分配。虽然目前还没实现对象池，但架构上支持。

## 可扩展性

整个架构是回调驱动的。用户代码通过回调函数接入，不需要继承或者修改框架代码。

各个模块职责清晰，耦合度低。比如 HTTP 层完全建立在 TCP 层之上，可以很容易替换成其他协议。

路由是动态注册的，可以在运行时添加或修改路由。

后续可以加中间件系统。在请求处理前后插入自定义逻辑，比如日志、认证、压缩等。

## 一些设计选择

为什么用 epoll？因为这个项目只跑在 Linux 上。如果要跨平台，可以用 poll 或者 select，但性能会差一些。

为什么是 one loop per thread？因为简单。不需要处理线程同步，代码更清晰。性能也不错，只要线程数合理。

为什么不用 coroutine？C++20 的 coroutine 还不够成熟。而且 callback 模式对于这种网络服务器已经足够了。

为什么不支持 io_uring？io_uring 是很新的技术，内核版本要求高。现在 epoll 已经够用了。后面可以加一个 IoUringPoller，跟 EpollPoller 并存。

## 总结

TZZero-HTTP 是个典型的 Reactor 模式服务器。用 one loop per thread 处理并发，用回调函数组织业务逻辑。各个模块职责明确，代码结构清晰。性能和可扩展性都不错。
