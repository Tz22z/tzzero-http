#include "tzzero/core/event_loop.h"
#include "tzzero/net/acceptor.h"
#include "tzzero/net/tcp_connection.h"
#include "tzzero/utils/buffer.h"
#include "tzzero/http/http_response.h"
#include <iostream>
#include <csignal>
#include <memory>
#include <map>

using namespace tzzero::core;
using namespace tzzero::net;
using namespace tzzero::http;

// 全局事件循环指针，用于信号处理
EventLoop* g_loop = nullptr;

void signal_handler(int sig) {
    if (g_loop) {
        std::cout << "\nReceived signal " << sig << ", shutting down..." << std::endl;
        g_loop->quit();
    }
}

// 创建一个标准的HTML页面
std::string create_welcome_page(const TcpConnectionPtr& conn) {
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
            <h3>连接信息:</h3>
            <p><strong>客户端地址:</strong> )" + conn->get_peer_address() + R"(</p>
            <p><strong>服务器地址:</strong> )" + conn->get_local_address() + R"(</p>
            <p><strong>连接名称:</strong> )" + conn->get_name() + R"(</p>
            <p><strong>状态:</strong> <span class="status">正常运行</span></p>
        </div>
        
        <h3>服务器特性:</h3>
        <div class="feature">✨ 事件驱动架构 - 基于Reactor模式</div>
        <div class="feature">🔥 高并发支持 - 非阻塞I/O处理</div>
        <div class="feature">💎 现代C++20技术 - 智能指针与RAII</div>
        <div class="feature">⚡ 专业HTTP响应 - 标准状态码和头部</div>
        <div class="feature">🎯 零拷贝优化 - 高性能缓冲区管理</div>
        
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

// HTTP服务器类
class SimpleHttpServer {
public:
    SimpleHttpServer(EventLoop* loop, const std::string& listen_addr, uint16_t port)
        : loop_(loop)
        , acceptor_(loop, listen_addr, port)
        , port_(port)
        , next_conn_id_(1)
    {
        acceptor_.set_new_connection_callback([this](int sockfd, const std::string& peer_addr) {
            handle_new_connection(sockfd, peer_addr);
        });
    }

    void start() {
        acceptor_.listen();
        std::cout << "HTTP Server listening on 0.0.0.0:" << port_ << std::endl;
    }

private:
    void handle_new_connection(int sockfd, const std::string& peer_addr) {
        std::cout << "New connection from " << peer_addr << std::endl;
        
        std::string conn_name = "HttpConn-" + std::to_string(next_conn_id_++);
        auto conn = std::make_shared<TcpConnection>(loop_, conn_name, sockfd);
        
        conn->set_message_callback([this](const TcpConnectionPtr& conn, tzzero::utils::Buffer& buffer) {
            handle_message(conn, buffer);
        });
        
        conn->set_close_callback([this](const TcpConnectionPtr& conn) {
            handle_close(conn);
        });
        
        connections_[conn_name] = conn;
        conn->connection_established();
    }
    
    void handle_message(const TcpConnectionPtr& conn, tzzero::utils::Buffer& buffer) {
        std::string request = buffer.retrieve_all_as_string();
        std::cout << "Received request from " << conn->get_peer_address() << ":\n" << request << std::endl;
        
        // 创建HttpResponse对象
        HttpResponse response;
        
        // 简单的路由处理
        if (request.find("GET /") == 0) {
            if (request.find("GET / ") == 0) {
                // 主页
                response.set_status_code(HttpStatusCode::OK);
                response.set_html_content_type();
                response.set_body(create_welcome_page(conn));
                
            } else if (request.find("GET /api/status") == 0) {
                // API状态接口
                response.set_status_code(HttpStatusCode::OK);
                response.set_json_content_type();
                response.set_body(R"({
    "server": "TZZero HTTP Server",
    "version": "1.0.0", 
    "status": "running",
    "features": ["HTTP/1.1", "Event-Driven", "C++20", "High-Performance"],
    "performance": {
        "architecture": "Reactor Pattern",
        "io_model": "Non-blocking I/O",
        "memory": "Zero-copy Buffer"
    }
})");
                
            } else if (request.find("GET /api/hello") == 0) {
                // API问候接口
                response.set_status_code(HttpStatusCode::OK);
                response.set_json_content_type();
                response.set_body(R"({
    "message": "Hello from TZZero HTTP Server!",
    "timestamp": ")" + std::to_string(time(nullptr)) + R"(",
    "client": ")" + conn->get_peer_address() + R"(",
    "server": ")" + conn->get_local_address() + R"(",
    "connection": ")" + conn->get_name() + R"("
})");
                
            } else if (request.find("GET /test") == 0) {
                // 测试页面
                response.set_status_code(HttpStatusCode::OK);
                response.set_html_content_type();
                response.set_body(R"(<!DOCTYPE html>
<html>
<head>
    <title>TZZero Test Page</title>
    <meta charset="UTF-8">
</head>
<body>
    <h1>🧪 TZZero HTTP Server 测试页面</h1>
    <p>这是一个测试页面，用于验证服务器功能。</p>
    <p><strong>服务器时间:</strong> )" + std::to_string(time(nullptr)) + R"(</p>
    <p><a href="/">← 返回首页</a></p>
</body>
</html>)");
                
            } else {
                // 404页面
                response.set_status_code(HttpStatusCode::NOT_FOUND);
                response.set_html_content_type();
                response.set_body(R"(<!DOCTYPE html>
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
        } else {
            // 不支持的方法
            response.set_status_code(HttpStatusCode::METHOD_NOT_ALLOWED);
            response.set_text_content_type();
            response.set_body("405 Method Not Allowed");
        }
        
        // 发送响应
        std::string response_data = response.to_buffer();
        conn->send(response_data);
        
        // 发送完毕后关闭连接
        conn->shutdown();
    }
    
    void handle_close(const TcpConnectionPtr& conn) {
        std::cout << "Connection closed: " << conn->get_name() << std::endl;
        connections_.erase(conn->get_name());
    }

    EventLoop* loop_;
    Acceptor acceptor_;
    uint16_t port_;
    std::map<std::string, TcpConnectionPtr> connections_;
    int next_conn_id_;
};

int main(int argc, char* argv[]) {
    std::cout << "TZZero HTTP Server Starting..." << std::endl;
    
    // 解析端口参数
    uint16_t port = 3000;  // 改为3000端口
    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }
    
    try {
        // 创建事件循环
        EventLoop loop;
        g_loop = &loop;
        
        // 设置信号处理
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        // 创建HTTP服务器
        SimpleHttpServer server(&loop, "0.0.0.0", port);
        
        // 启动服务器
        server.start();
        
        // 添加状态报告定时器
        loop.run_every(30.0, []() {
            std::cout << "Server status: running normally..." << std::endl;
        });
        
        std::cout << "Server started! Visit http://localhost:" << port << " to test" << std::endl;
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
