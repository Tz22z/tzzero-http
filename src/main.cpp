#include "tzzero/core/event_loop.h"
#include "tzzero/http/http_server.h"
#include "tzzero/http/http_request.h"
#include "tzzero/http/http_response.h"
#include <iostream>
#include <csignal>
#include <ctime>

using namespace tzzero::core;
using namespace tzzero::http;

// 全局事件循环指针，用于信号处理
EventLoop* g_loop = nullptr;

void signal_handler(int sig) {
    if (g_loop) {
        std::cout << "\nReceived signal " << sig << ", shutting down..." << std::endl;
        g_loop->quit();
    }
}

// 创建欢迎页面
std::string create_welcome_page() {
    return R"(<!DOCTYPE html>
<html>
<head>
    <title>TZZero HTTP Server</title>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 20px; }
        .info { background: #ecf0f1; padding: 20px; border-radius: 5px; margin: 20px 0; }
        .feature { background: #e8f5e8; padding: 15px; margin: 10px 0; border-left: 4px solid #27ae60; }
        .status { color: #27ae60; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1 class="header">🚀 欢迎使用 TZZero HTTP 服务器!</h1>

        <div class="info">
            <h3>服务器信息:</h3>
            <p><strong>版本:</strong> 1.0.0</p>
            <p><strong>状态:</strong> <span class="status">正常运行</span></p>
        </div>

        <h3>服务器特性:</h3>
        <div class="feature">✨ 事件驱动架构 - 基于Reactor模式</div>
        <div class="feature">🔥 高并发支持 - 多线程EventLoop池</div>
        <div class="feature">💎 现代C++20技术 - 智能指针与RAII</div>
        <div class="feature">⚡ 专业HTTP解析 - 完整协议支持</div>
        <div class="feature">🎯 零拷贝优化 - 高性能缓冲区管理</div>
        <div class="feature">🔄 Keep-Alive支持 - 连接复用优化</div>

        <div class="info">
            <h3>测试接口:</h3>
            <p>• <a href="/api/status">GET /api/status</a> - 服务器状态</p>
            <p>• <a href="/api/hello">GET /api/hello</a> - JSON问候</p>
            <p>• <a href="/test">GET /test</a> - 测试页面</p>
        </div>
    </div>
</body>
</html>)";
}

// HTTP请求处理器
void http_handler(const HttpRequest& req, HttpResponse& resp) {
    const std::string& path = req.get_path();

    if (path == "/") {
        // 主页
        resp.set_status_code(HttpStatusCode::OK);
        resp.set_html_content_type();
        resp.set_body(create_welcome_page());

    } else if (path == "/api/status") {
        // API状态接口
        resp.set_status_code(HttpStatusCode::OK);
        resp.set_json_content_type();
        resp.set_body(R"({
    "server": "TZZero HTTP Server",
    "version": "1.0.0",
    "status": "running",
    "features": [
        "HTTP/1.1",
        "Event-Driven",
        "Multi-threaded",
        "Keep-Alive",
        "C++20",
        "High-Performance"
    ],
    "architecture": {
        "pattern": "Reactor",
        "io_model": "Non-blocking I/O + Epoll",
        "threading": "EventLoop Thread Pool",
        "memory": "Zero-copy Buffer"
    }
})");

    } else if (path == "/api/hello") {
        // API问候接口
        resp.set_status_code(HttpStatusCode::OK);
        resp.set_json_content_type();
        resp.set_body(R"({
    "message": "Hello from TZZero HTTP Server!",
    "timestamp": )" + std::to_string(time(nullptr)) + R"(,
    "protocol": "HTTP/1.1",
    "features": ["Keep-Alive", "Multi-threaded", "Event-Driven"]
})");

    } else if (path == "/test") {
        // 测试页面
        resp.set_status_code(HttpStatusCode::OK);
        resp.set_html_content_type();
        resp.set_body(R"(<!DOCTYPE html>
<html>
<head>
    <title>TZZero Test Page</title>
    <meta charset="UTF-8">
</head>
<body>
    <h1>🧪 TZZero HTTP Server 测试页面</h1>
    <p>这是一个测试页面，用于验证服务器功能。</p>
    <p><strong>服务器时间:</strong> )" + std::to_string(time(nullptr)) + R"(</p>
    <p><strong>请求路径:</strong> )" + req.get_path() + R"(</p>
    <p><strong>HTTP方法:</strong> )" + req.get_method_string() + R"(</p>
    <p><strong>HTTP版本:</strong> )" + req.get_version_string() + R"(</p>
    <p><a href="/">← 返回首页</a></p>
</body>
</html>)");

    } else {
        // 404页面
        resp.set_status_code(HttpStatusCode::NOT_FOUND);
        resp.set_html_content_type();
        resp.set_body(R"(<!DOCTYPE html>
<html>
<head>
    <title>404 - Page Not Found</title>
    <meta charset="UTF-8">
</head>
<body>
    <h1>404 - 页面未找到</h1>
    <p>抱歉，您请求的页面不存在。</p>
    <p><a href="/">返回首页</a> | <a href="/api/status">服务器状态</a></p>
</body>
</html>)");
    }
}

int main(int argc, char* argv[]) {
    std::cout << "TZZero HTTP Server Starting..." << std::endl;

    // 解析端口参数
    uint16_t port = 3000;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }

    // 解析线程数参数
    int thread_num = 4;  // 默认4个工作线程
    if (argc > 2) {
        thread_num = std::atoi(argv[2]);
    }

    try {
        // 创建事件循环
        EventLoop loop;
        g_loop = &loop;

        // 设置信号处理
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 创建HTTP服务器
        HttpServer server(&loop, "0.0.0.0", port, "TZZeroHTTP");

        // 配置服务器
        server.set_thread_num(thread_num);
        server.enable_keep_alive(true);
        server.set_keep_alive_timeout(60);

        // 设置HTTP请求处理器
        server.set_http_callback(http_handler);

        // 启动服务器
        server.start();

        // 添加状态报告定时器
        loop.run_every(30.0, []() {
            std::cout << "Server status: running normally..." << std::endl;
        });

        std::cout << "Server started on port " << port << " with " << thread_num << " worker threads" << std::endl;
        std::cout << "Visit http://localhost:" << port << " to test" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        // 启动事件循环
        loop.loop();

        std::cout << "Event loop stopped" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "TZZero HTTP Server stopped" << std::endl;
    return 0;
}
