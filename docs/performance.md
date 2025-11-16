# 性能优化指南

本文档介绍 TZZero-HTTP 的性能调优方法和最佳实践。

## 性能指标

### 关键指标

- **吞吐量 (Throughput)**: requests/second
- **延迟 (Latency)**: P50, P90, P99, P999
- **并发连接数 (Concurrent Connections)**
- **CPU 使用率**
- **内存使用量**

### 基准测试

典型硬件（4 核 CPU，8GB 内存）上的性能：

| 场景 | QPS | P99 延迟 | 并发数 |
|------|-----|----------|--------|
| 静态响应 (100 字节) | 150K+ | <5ms | 1000 |
| JSON API | 100K+ | <10ms | 1000 |
| 文件服务 (1KB) | 80K+ | <15ms | 1000 |
| 文件服务 (1MB) | 2K+ | <100ms | 100 |

## 配置优化

### 1. 线程池大小

```cpp
// CPU 密集型：线程数 = CPU 核心数
server.set_thread_num(std::thread::hardware_concurrency());

// I/O 密集型：线程数 = CPU 核心数 * 2
server.set_thread_num(std::thread::hardware_concurrency() * 2);

// 混合负载：根据实际情况调整
server.set_thread_num(4);  // 从小开始，逐步增加
```

### 2. 缓冲区大小

```cpp
// 增加初始缓冲区大小（减少动态扩容）
Buffer buffer(4096);  // 默认 1024

// 设置高水位标记（防止内存过度使用）
server.set_high_water_mark(64 * 1024 * 1024);  // 64MB
```

### 3. TCP 参数

```cpp
// 启用 TCP_NODELAY（减少延迟）
server.enable_tcp_nodelay(true);

// 设置 SO_REUSEADDR 和 SO_REUSEPORT
server.set_reuse_addr(true);
server.set_reuse_port(true);  // Linux 3.9+

// 设置接收/发送缓冲区大小
server.set_recv_buffer_size(256 * 1024);  // 256KB
server.set_send_buffer_size(256 * 1024);
```

### 4. 系统参数

```bash
# 增加最大文件描述符数
ulimit -n 65535

# 调整内核参数
sudo sysctl -w net.core.somaxconn=65535
sudo sysctl -w net.ipv4.tcp_max_syn_backlog=8192
sudo sysctl -w net.ipv4.ip_local_port_range="1024 65535"

# 启用 TCP fast open
sudo sysctl -w net.ipv4.tcp_fastopen=3
```

## 代码优化

### 1. 避免内存分配

```cpp
// 坏：频繁分配
for (int i = 0; i < 1000000; ++i) {
    std::string temp = "Hello";  // 每次都分配
    process(temp);
}

// 好：复用对象
std::string temp;
for (int i = 0; i < 1000000; ++i) {
    temp = "Hello";
    process(temp);
}

// 更好：使用对象池
MemoryPool<MyObject> pool(1000);
auto* obj = pool.allocate();
// 使用完后
pool.deallocate(obj);
```

### 2. 使用移动语义

```cpp
// 坏：拷贝
std::string generate_response() {
    std::string response = "...";
    return response;
}

// 好：移动（编译器优化）
std::string generate_response() {
    return "...";  // NRVO 或移动
}

// 手动移动
resp.set_body(std::move(large_string));
```

### 3. 预分配和保留空间

```cpp
// 预分配容器
std::vector<int> vec;
vec.reserve(1000);  // 避免多次扩容

// 预分配字符串
std::string str;
str.reserve(4096);
```

### 4. 使用 string_view

```cpp
// 坏：拷贝子字符串
std::string extract(const std::string& str) {
    return str.substr(0, 10);  // 拷贝
}

// 好：使用视图
std::string_view extract(std::string_view str) {
    return str.substr(0, 10);  // 无拷贝
}
```

### 5. 减少系统调用

```cpp
// 坏：多次 write
write(fd, data1, len1);
write(fd, data2, len2);
write(fd, data3, len3);

// 好：使用 writev（批量写）
struct iovec iov[3] = {
    {data1, len1},
    {data2, len2},
    {data3, len3}
};
writev(fd, iov, 3);

// TZZero-HTTP 自动使用 writev
buffer.append(header);
buffer.append(body);
buffer.write_fd(fd);  // 一次系统调用
```

## 架构优化

### 1. 无锁编程

```cpp
// 坏：共享数据 + 锁
std::mutex mtx;
int counter = 0;

void increment() {
    std::lock_guard<std::mutex> lock(mtx);
    ++counter;  // 锁竞争
}

// 好：每线程数据
thread_local int counter = 0;

void increment() {
    ++counter;  // 无锁
}

// 最好：使用 atomic（如果必须共享）
std::atomic<int> counter{0};

void increment() {
    counter.fetch_add(1, std::memory_order_relaxed);
}
```

### 2. 数据局部性

```cpp
// 坏：指针跳跃
struct Node {
    int value;
    Node* next;
};
// 遍历链表：缓存不友好

// 好：连续内存
std::vector<int> values;
// 遍历数组：缓存友好
```

### 3. 批处理

```cpp
// 坏：逐个处理
for (auto& req : requests) {
    process(req);
    send_response(req);
}

// 好：批量处理
std::vector<Response> responses;
for (auto& req : requests) {
    responses.push_back(process(req));
}
batch_send(responses);  // 批量发送
```

## 性能分析工具

### 1. perf

```bash
# CPU 分析
perf record -g ./tzzero-http
perf report

# 查看热点函数
perf top

# 缓存命中率
perf stat -e cache-references,cache-misses ./tzzero-http
```

### 2. Flamegraph

```bash
# 生成火焰图
perf record -F 99 -g ./tzzero-http
perf script > out.perf
stackcollapse-perf.pl out.perf > out.folded
flamegraph.pl out.folded > flame.svg
```

### 3. gperftools

```cpp
#include <gperftools/profiler.h>

ProfilerStart("profile.prof");
server.start();
ProfilerStop();
```

```bash
# 分析
google-pprof --text ./tzzero-http profile.prof
google-pprof --web ./tzzero-http profile.prof
```

### 4. Valgrind Callgrind

```bash
valgrind --tool=callgrind ./tzzero-http
kcachegrind callgrind.out.*
```

## 压力测试

### 使用 wrk

```bash
# 基本测试
wrk -t4 -c100 -d30s http://localhost:8080/

# 自定义脚本
wrk -t4 -c100 -d30s -s post.lua http://localhost:8080/api
```

post.lua:
```lua
wrk.method = "POST"
wrk.body   = '{"key":"value"}'
wrk.headers["Content-Type"] = "application/json"
```

### 使用 Apache Bench

```bash
ab -n 100000 -c 1000 -k http://localhost:8080/
```

### 使用 hey

```bash
hey -n 100000 -c 100 -m GET http://localhost:8080/
```

## 监控

### 1. 实时监控

```cpp
// 定期输出统计信息
server.set_stats_callback([](const ServerStats& stats) {
    LOG_INFO("Connections: " << stats.active_connections);
    LOG_INFO("Requests/s: " << stats.qps);
    LOG_INFO("Memory: " << stats.memory_mb << " MB");
});
```

### 2. Prometheus 指标

```cpp
// 暴露 /metrics 端点
server.route("/metrics", [](const HttpRequest& req, HttpResponse& resp) {
    resp.set_text_content_type();
    resp.set_body(
        "# TYPE http_requests_total counter\n"
        "http_requests_total " + std::to_string(request_counter) + "\n"
        "# TYPE http_request_duration_seconds histogram\n"
        "http_request_duration_seconds_sum " + std::to_string(total_duration) + "\n"
    );
});
```

## 常见性能问题

### 1. 高延迟

**症状**: P99 延迟过高

**原因**:
- GC 暂停（C++ 无此问题）
- 锁竞争
- 磁盘 I/O 阻塞
- 网络拥塞

**解决**:
- 使用异步 I/O
- 减少锁的使用
- 使用 SSD
- 启用 TCP_NODELAY

### 2. 低吞吐量

**症状**: QPS 低于预期

**原因**:
- 线程数不足
- CPU 瓶颈
- 系统调用过多

**解决**:
- 增加工作线程
- 代码优化（见上文）
- 批量操作

### 3. 高内存使用

**症状**: 内存持续增长

**原因**:
- 内存泄漏
- 缓冲区过大
- 连接泄漏

**解决**:
- 使用 Valgrind 检测泄漏
- 设置高水位标记
- 正确关闭连接

## 最佳实践总结

1. **选择合适的线程数**: CPU 核心数到 2 倍之间
2. **复用对象**: 使用对象池和内存池
3. **减少拷贝**: 使用移动语义和 string_view
4. **批量操作**: 批量读写，减少系统调用
5. **无锁设计**: One Loop Per Thread 模式
6. **数据局部性**: 连续内存，缓存友好
7. **异步 I/O**: 避免阻塞操作
8. **监控和分析**: 使用工具持续优化

## 性能检查清单

在部署前确保：

- [ ] 使用 Release 模式编译
- [ ] 设置合适的线程数
- [ ] 调整系统参数（ulimit, sysctl）
- [ ] 进行压力测试
- [ ] 分析性能瓶颈
- [ ] 监控资源使用
- [ ] 准备扩容方案

---

性能优化是持续的过程，需要不断测试、分析和改进。
