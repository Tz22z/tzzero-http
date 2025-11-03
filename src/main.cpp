#include "tzzero/core/event_loop.h"
#include "tzzero/http/http_server.h"
#include "tzzero/http/http_request.h"
#include "tzzero/http/http_response.h"
#include "tzzero/utils/logger.h"
#include <iostream>
#include <csignal>
#include <ctime>
#include <getopt.h>
#include <thread>

using namespace tzzero::core;
using namespace tzzero::http;
using namespace tzzero::utils;

// 全局指针，用于信号处理
EventLoop* g_loop = nullptr;
HttpServer* g_server = nullptr;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\n正在优雅关闭服务器..." << std::endl;
        if (g_server) {
            g_server->stop();
        }
        if (g_loop) {
            g_loop->quit();
        }
    }
}

void setup_signal_handlers() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); // 忽略SIGPIPE
}

void print_usage(const char* program_name) {
    std::cout << "用法: " << program_name << " [选项]\n"
              << "选项:\n"
              << "  -h, --help           显示帮助信息\n"
              << "  -p, --port PORT      监听端口 (默认: 3000)\n"
              << "  -a, --addr ADDR      监听地址 (默认: 0.0.0.0)\n"
              << "  -t, --threads NUM    工作线程数 (默认: CPU核心数)\n"
              << "  -k, --keepalive      启用HTTP keep-alive (默认: 启用)\n"
              << "  -l, --log-file FILE  日志输出文件 (默认: 仅控制台)\n"
              << "  -L, --log-level LVL  日志级别: DEBUG, INFO, WARN, ERROR (默认: INFO)\n"
              << "  -v, --verbose        详细输出\n"
              << std::endl;
}

// 创建欢迎页面
std::string create_welcome_page() {
    return R"(<!DOCTYPE html>
<html>
<head>
    <title>TZZero HTTP Server</title>
</head>
<body>
    <h1>TZZero HTTP Server</h1>
    <p>Server is running.</p>
    <ul>
        <li><a href="/api/status">Status API</a></li>
        <li><a href="/api/hello">Hello API</a></li>
        <li><a href="/test">Test Page</a></li>
    </ul>
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
    "status": "ok",
    "version": "1.0.0"
})");

    } else if (path == "/api/hello") {
        // API问候接口
        resp.set_status_code(HttpStatusCode::OK);
        resp.set_json_content_type();
        resp.set_body(R"({
    "message": "hello"
})");

    } else if (path == "/test") {
        // 测试页面
        resp.set_status_code(HttpStatusCode::OK);
        resp.set_html_content_type();
        resp.set_body(R"(<!DOCTYPE html>
<html>
<head>
    <title>Test Page</title>
</head>
<body>
    <h1>Test Page</h1>
    <p>Method: )" + req.get_method_string() + R"(</p>
    <p>Path: )" + req.get_path() + R"(</p>
    <p><a href="/">Home</a></p>
</body>
</html>)");

    } else {
        // 404页面
        resp.set_status_code(HttpStatusCode::NOT_FOUND);
        resp.set_html_content_type();
        resp.set_body(R"(<!DOCTYPE html>
<html>
<head>
    <title>404 Not Found</title>
</head>
<body>
    <h1>404 Not Found</h1>
    <p><a href="/">Home</a></p>
</body>
</html>)");
    }
}

int main(int argc, char* argv[]) {
    // 默认配置
    std::string listen_addr = "0.0.0.0";
    uint16_t port = 3000;
    int thread_num = std::thread::hardware_concurrency();
    bool enable_keepalive = true;
    bool verbose = false;
    std::string log_file;
    std::string log_level = "INFO";

    // 命令行参数解析
    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"addr", required_argument, 0, 'a'},
        {"threads", required_argument, 0, 't'},
        {"keepalive", no_argument, 0, 'k'},
        {"log-file", required_argument, 0, 'l'},
        {"log-level", required_argument, 0, 'L'},
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "hp:a:t:kl:L:v", long_options, nullptr)) != -1) {
        switch (c) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'p':
                port = static_cast<uint16_t>(std::stoi(optarg));
                break;
            case 'a':
                listen_addr = optarg;
                break;
            case 't':
                thread_num = std::stoi(optarg);
                break;
            case 'k':
                enable_keepalive = true;
                break;
            case 'l':
                log_file = optarg;
                break;
            case 'L':
                log_level = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    try {
        // 配置日志系统
        auto& logger = Logger::instance();

        // 设置日志级别
        if (log_level == "DEBUG") logger.set_level(LogLevel::DEBUG);
        else if (log_level == "INFO") logger.set_level(LogLevel::INFO);
        else if (log_level == "WARN") logger.set_level(LogLevel::WARN);
        else if (log_level == "ERROR") logger.set_level(LogLevel::ERROR);
        else {
            std::cerr << "无效的日志级别: " << log_level << std::endl;
            return 1;
        }

        // 设置日志文件（如果指定）
        if (!log_file.empty()) {
            logger.set_output_file(log_file);
            logger.set_max_file_size(100); // 100MB
            logger.set_max_files(10);
        }

        // 设置信号处理器
        setup_signal_handlers();

        std::cout << "TZZero HTTP 服务器启动中..." << std::endl;

        // 创建事件循环
        EventLoop loop;
        g_loop = &loop;

        // 创建HTTP服务器
        HttpServer server(&loop, listen_addr, port, "TZZeroHTTP");
        g_server = &server;

        // 配置服务器
        server.set_thread_num(thread_num);
        server.enable_keep_alive(enable_keepalive);
        server.set_keep_alive_timeout(60);

        // 设置HTTP请求处理器
        server.set_http_callback(http_handler);

        // 启动服务器
        server.start();

        // 添加状态报告定时器
        loop.run_every(30.0, []() {
            std::cout << "服务器状态: 运行正常..." << std::endl;
        });

        std::cout << "服务器已启动在 " << listen_addr << ":" << port
                  << ", 使用 " << thread_num << " 个工作线程" << std::endl;
        std::cout << "访问 http://localhost:" << port << " 进行测试" << std::endl;
        std::cout << "按 Ctrl+C 停止" << std::endl;

        // 启动事件循环
        loop.loop();

        std::cout << "事件循环已停止" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "TZZero HTTP 服务器已停止" << std::endl;
    return 0;
}
