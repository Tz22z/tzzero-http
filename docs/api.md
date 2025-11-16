# TZZero-HTTP API 文档

本文档提供 TZZero-HTTP 框架的完整 API 参考。

## 目录

- [HTTP 模块](#http-模块)
- [网络模块](#网络模块)
- [核心模块](#核心模块)
- [工具模块](#工具模块)

---

## HTTP 模块

### HttpServer

HTTP 服务器主类，提供 HTTP 服务功能。

#### 构造函数

```cpp
HttpServer(const std::string& address, uint16_t port, int thread_num = 0);
```

**参数**:
- `address`: 监听地址（如 "0.0.0.0"）
- `port`: 监听端口
- `thread_num`: 工作线程数（0 表示使用 CPU 核心数）

#### 核心方法

```cpp
void start();
void stop();
```

启动和停止服务器。

#### 路由方法

```cpp
void route(const std::string& path, RouteCallback callback);
```

注册精确路径路由。

**示例**:
```cpp
server.route("/api/users", [](const HttpRequest& req, HttpResponse& resp) {
    resp.set_json_content_type();
    resp.set_body("{\"users\": []}");
});
```

```cpp
void route_pattern(const std::string& prefix, RouteCallback callback);
```

注册前缀匹配路由。

**示例**:
```cpp
server.route_pattern("/api/user/", [](const HttpRequest& req, HttpResponse& resp) {
    // 处理 /api/user/1, /api/user/123 等
});
```

```cpp
void set_default_handler(RouteCallback callback);
```

设置默认处理器（处理所有未匹配的请求）。

---

### HttpRequest

HTTP 请求类。

#### 请求行

```cpp
HttpMethod get_method() const;
void set_method(HttpMethod method);
std::string get_method_string() const;

const std::string& get_path() const;
void set_path(const std::string& path);

const std::string& get_query() const;
void set_query(const std::string& query);

HttpVersion get_version() const;
void set_version(HttpVersion version);
std::string get_version_string() const;
```

#### 头部字段

```cpp
void add_header(const std::string& field, const std::string& value);
void set_header(const std::string& field, const std::string& value);
std::string get_header(const std::string& field) const;
bool has_header(const std::string& field) const;
void remove_header(const std::string& field);
const std::unordered_map<std::string, std::string>& get_headers() const;
```

#### 请求体

```cpp
void set_body(const std::string& body);
void set_body(std::string&& body);
const std::string& get_body() const;
size_t get_content_length() const;
```

#### 其他方法

```cpp
bool keep_alive() const;  // 是否保持连接
void reset();             // 重置请求对象
```

#### 枚举类型

```cpp
enum class HttpMethod {
    INVALID, GET, POST, PUT, DELETE,
    HEAD, OPTIONS, PATCH, CONNECT, TRACE
};

enum class HttpVersion {
    UNKNOWN, HTTP_1_0, HTTP_1_1, HTTP_2_0
};
```

---

### HttpResponse

HTTP 响应类。

#### 状态码

```cpp
void set_status_code(HttpStatusCode code);
HttpStatusCode get_status_code() const;
std::string get_status_message() const;
```

#### 头部字段

```cpp
void add_header(const std::string& field, const std::string& value);
void set_header(const std::string& field, const std::string& value);
std::string get_header(const std::string& field) const;
bool has_header(const std::string& field) const;
void remove_header(const std::string& field);
```

#### 响应体

```cpp
void set_body(const std::string& body);
void set_body(std::string&& body);
void append_body(const std::string& data);
const std::string& get_body() const;
void clear_body();
```

#### Content-Type 快捷方法

```cpp
void set_content_type(const std::string& content_type);
void set_json_content_type();   // application/json; charset=utf-8
void set_html_content_type();   // text/html; charset=utf-8
void set_text_content_type();   // text/plain; charset=utf-8
```

#### 重定向

```cpp
void redirect(const std::string& url, HttpStatusCode code = HttpStatusCode::FOUND);
```

#### 其他方法

```cpp
void set_close_connection(bool close);
bool close_connection() const;
void reset();
std::string to_buffer() const;
```

#### 状态码枚举

```cpp
enum class HttpStatusCode {
    // 2xx Success
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,

    // 3xx Redirection
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    NOT_MODIFIED = 304,

    // 4xx Client Error
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,

    // 5xx Server Error
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};
```

---

### HttpParser

HTTP 协议解析器。

```cpp
class HttpParser {
public:
    HttpParser();

    bool parse_request(utils::Buffer& buffer, HttpRequest& request);
    void reset();
    bool has_error() const;
};
```

---

## 工具模块

### Buffer

高性能网络缓冲区。

#### 构造函数

```cpp
explicit Buffer(size_t initial_size = kInitialSize);
```

#### 大小和容量

```cpp
size_t readable_bytes() const;    // 可读字节数
size_t writable_bytes() const;    // 可写字节数
size_t prependable_bytes() const; // prepend 空间大小
size_t capacity() const;          // 总容量
```

#### 读取操作

```cpp
const char* peek() const;  // 查看数据（不移动读指针）
void retrieve(size_t len); // 移动读指针
void retrieve_all();       // 清空缓冲区
std::string retrieve_as_string(size_t len);
std::string retrieve_all_as_string();
```

#### 写入操作

```cpp
void append(const char* data, size_t len);
void append(const std::string& str);
void append(std::string_view str);
```

#### 整数操作（网络字节序）

```cpp
void append_int8(int8_t x);
void append_int16(int16_t x);
void append_int32(int32_t x);
void append_int64(int64_t x);

int8_t read_int8();
int16_t read_int16();
int32_t read_int32();
int64_t read_int64();

int8_t peek_int8() const;
int16_t peek_int16() const;
int32_t peek_int32() const;
int64_t peek_int64() const;
```

#### 查找操作

```cpp
const char* find_crlf() const;                // 查找 \r\n
const char* find_crlf(const char* start) const;
const char* find_eol() const;                 // 查找 \n
const char* find_eol(const char* start) const;
```

#### I/O 操作

```cpp
ssize_t read_fd(int fd, int* saved_errno);   // 从文件描述符读取
ssize_t write_fd(int fd, int* saved_errno);  // 写入文件描述符
```

---

### Logger

线程安全的日志系统。

#### 获取实例

```cpp
static Logger& instance();
```

#### 日志级别

```cpp
void set_level(LogLevel level);
LogLevel get_level() const;

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};
```

#### 文件输出

```cpp
void set_output_file(const std::string& filename);
void set_max_file_size(size_t max_size_mb);  // 默认 100MB
void set_max_files(int max_files);           // 默认 10 个
```

#### 日志方法

```cpp
void log(LogLevel level, const std::string& message);
void log(LogLevel level, const char* file, int line, const std::string& message);
```

#### 便捷宏

```cpp
LOG_DEBUG(msg);   // 调试信息
LOG_INFO(msg);    // 一般信息
LOG_WARN(msg);    // 警告
LOG_ERROR(msg);   // 错误
LOG_FATAL(msg);   // 致命错误
```

**示例**:
```cpp
LOG_INFO("Server started on port " << port);
LOG_ERROR("Failed to connect: " << error_msg);
```

---

### ThreadPool

通用线程池。

#### 构造函数

```cpp
explicit ThreadPool(size_t num_threads);
```

#### 提交任务

```cpp
template<typename F, typename... Args>
auto submit(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>;
```

**示例**:
```cpp
ThreadPool pool(4);

// 提交无返回值任务
auto future1 = pool.submit([]() {
    std::cout << "Task running\n";
});

// 提交有返回值任务
auto future2 = pool.submit([](int a, int b) {
    return a + b;
}, 10, 20);

int result = future2.get();  // 获取结果：30
```

#### 其他方法

```cpp
size_t size() const;  // 获取线程数
void stop();          // 停止线程池
```

---

### MemoryPool

内存池模板类。

#### 构造函数

```cpp
template<typename T>
explicit MemoryPool(size_t block_size = 1024);
```

#### 分配和释放

```cpp
T* allocate();              // 分配对象
void deallocate(T* ptr);    // 释放对象
```

#### 统计信息

```cpp
size_t allocated_count() const;  // 已分配对象数
size_t total_capacity() const;   // 总容量
```

**示例**:
```cpp
MemoryPool<MyClass> pool(100);

MyClass* obj1 = pool.allocate();
MyClass* obj2 = pool.allocate();

// 使用对象...

pool.deallocate(obj1);
pool.deallocate(obj2);
```

---

## 网络模块

### TcpServer

TCP 服务器（由 HttpServer 内部使用）。

```cpp
class TcpServer {
public:
    TcpServer(EventLoop* loop, const InetAddress& listen_addr);

    void start();
    void stop();
    void set_thread_num(int num_threads);

    void set_connection_callback(const ConnectionCallback& cb);
    void set_message_callback(const MessageCallback& cb);
    void set_write_complete_callback(const WriteCompleteCallback& cb);
};
```

### TcpConnection

TCP 连接（由框架内部管理）。

```cpp
class TcpConnection {
public:
    void send(const std::string& message);
    void send(Buffer* buf);
    void shutdown();

    const std::string& name() const;
    bool connected() const;
    const InetAddress& local_address() const;
    const InetAddress& peer_address() const;
};
```

---

## 核心模块

### EventLoop

事件循环（高级用法）。

```cpp
class EventLoop {
public:
    void loop();
    void quit();

    void run_in_loop(Functor cb);
    void queue_in_loop(Functor cb);

    TimerId run_at(Timestamp time, TimerCallback cb);
    TimerId run_after(double delay, TimerCallback cb);
    TimerId run_every(double interval, TimerCallback cb);

    bool is_in_loop_thread() const;
};
```

---

## 回调函数类型

```cpp
// HTTP 回调
using RouteCallback = std::function<void(const HttpRequest&, HttpResponse&)>;

// TCP 回调
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

// 定时器回调
using TimerCallback = std::function<void()>;

// 通用回调
using Functor = std::function<void()>;
```

---

## 使用示例

### 完整的 HTTP 服务器

```cpp
#include "tzzero/http/http_server.h"
#include "tzzero/utils/logger.h"

using namespace tzzero::http;
using namespace tzzero::utils;

int main() {
    // 配置日志
    Logger::instance().set_level(LogLevel::INFO);
    Logger::instance().set_output_file("server.log");

    // 创建服务器
    HttpServer server("0.0.0.0", 8080, 4);  // 4 个工作线程

    // 注册路由
    server.route("/", [](const HttpRequest& req, HttpResponse& resp) {
        resp.set_html_content_type();
        resp.set_body("<h1>Welcome</h1>");
    });

    server.route("/api/data", [](const HttpRequest& req, HttpResponse& resp) {
        if (req.get_method() == HttpMethod::GET) {
            resp.set_json_content_type();
            resp.set_body("{\"status\": \"ok\"}");
        } else {
            resp.set_status_code(HttpStatusCode::METHOD_NOT_ALLOWED);
        }
    });

    // 404 处理
    server.set_default_handler([](const HttpRequest& req, HttpResponse& resp) {
        resp.set_status_code(HttpStatusCode::NOT_FOUND);
        resp.set_html_content_type();
        resp.set_body("<h1>404 Not Found</h1>");
    });

    LOG_INFO("Server starting on http://0.0.0.0:8080");

    // 启动服务器（阻塞）
    server.start();

    return 0;
}
```

---

## 注意事项

1. **线程安全**: 大部分类不是线程安全的，使用 `run_in_loop()` 跨线程操作
2. **对象生命周期**: TcpConnection 由智能指针管理，注意避免循环引用
3. **回调中不要阻塞**: 回调函数应该快速返回，耗时操作提交到线程池
4. **Buffer 管理**: 注意 Buffer 的 retrieve 操作会移动读指针

---

## 更多资源

- [架构设计文档](architecture.md)
- [示例代码](../examples/)
- [GitHub Issues](https://github.com/yourusername/tzzero-http/issues)
