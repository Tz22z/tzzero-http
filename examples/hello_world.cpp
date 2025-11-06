/*
 * 简单的 Hello World HTTP 服务器
 * 演示最基本的服务器使用方式
 */

#include "tzzero/http/http_server.h"
#include "tzzero/http/http_request.h"
#include "tzzero/http/http_response.h"
#include "tzzero/utils/logger.h"
#include <iostream>

using namespace tzzero::http;
using namespace tzzero::utils;

int main() {
    // 设置日志级别
    Logger::instance().set_level(LogLevel::INFO);

    // 创建 HTTP 服务器
    HttpServer server("0.0.0.0", 8080);

    // 注册根路径处理器
    server.route("/", [](const HttpRequest& req, HttpResponse& resp) {
        resp.set_status_code(HttpStatusCode::OK);
        resp.set_html_content_type();
        resp.set_body("<html><body><h1>Hello, World!</h1></body></html>");
    });

    // 注册 /api/hello 路径
    server.route("/api/hello", [](const HttpRequest& req, HttpResponse& resp) {
        resp.set_status_code(HttpStatusCode::OK);
        resp.set_json_content_type();
        resp.set_body("{\"message\": \"Hello, World!\"}");
    });

    std::cout << "Server starting on http://0.0.0.0:8080" << std::endl;
    std::cout << "Try:" << std::endl;
    std::cout << "  curl http://localhost:8080/" << std::endl;
    std::cout << "  curl http://localhost:8080/api/hello" << std::endl;

    // 启动服务器
    server.start();

    return 0;
}
