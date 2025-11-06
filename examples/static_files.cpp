/*
 * 静态文件服务器示例
 * 演示如何提供静态文件服务
 */

#include "tzzero/http/http_server.h"
#include "tzzero/http/http_request.h"
#include "tzzero/http/http_response.h"
#include "tzzero/utils/logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

using namespace tzzero::http;
using namespace tzzero::utils;
namespace fs = std::filesystem;

// 根据文件扩展名获取 Content-Type
std::string get_content_type(const std::string& path) {
    if (path.ends_with(".html") || path.ends_with(".htm")) {
        return "text/html; charset=utf-8";
    } else if (path.ends_with(".css")) {
        return "text/css; charset=utf-8";
    } else if (path.ends_with(".js")) {
        return "application/javascript; charset=utf-8";
    } else if (path.ends_with(".json")) {
        return "application/json; charset=utf-8";
    } else if (path.ends_with(".png")) {
        return "image/png";
    } else if (path.ends_with(".jpg") || path.ends_with(".jpeg")) {
        return "image/jpeg";
    } else if (path.ends_with(".gif")) {
        return "image/gif";
    } else if (path.ends_with(".svg")) {
        return "image/svg+xml";
    } else if (path.ends_with(".txt")) {
        return "text/plain; charset=utf-8";
    }
    return "application/octet-stream";
}

// 读取文件内容
bool read_file(const std::string& path, std::string& content) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    content = oss.str();
    return true;
}

int main(int argc, char* argv[]) {
    Logger::instance().set_level(LogLevel::INFO);

    // 默认工作目录是当前目录
    std::string root_dir = ".";
    if (argc > 1) {
        root_dir = argv[1];
    }

    if (!fs::exists(root_dir) || !fs::is_directory(root_dir)) {
        std::cerr << "Error: " << root_dir << " is not a valid directory" << std::endl;
        return 1;
    }

    HttpServer server("0.0.0.0", 8080);

    // 处理所有请求
    server.set_default_handler([root_dir](const HttpRequest& req, HttpResponse& resp) {
        std::string path = req.get_path();

        // 安全检查：防止目录遍历攻击
        if (path.find("..") != std::string::npos) {
            resp.set_status_code(HttpStatusCode::FORBIDDEN);
            resp.set_text_content_type();
            resp.set_body("403 Forbidden");
            return;
        }

        // 构建完整文件路径
        std::string full_path = root_dir + path;

        // 如果是目录，尝试提供 index.html
        if (fs::is_directory(full_path)) {
            full_path += "/index.html";
        }

        // 读取并返回文件
        std::string content;
        if (read_file(full_path, content)) {
            resp.set_status_code(HttpStatusCode::OK);
            resp.set_content_type(get_content_type(full_path));
            resp.set_body(std::move(content));

            LOG_INFO("Served: " << path << " (" << content.size() << " bytes)");
        } else {
            resp.set_status_code(HttpStatusCode::NOT_FOUND);
            resp.set_html_content_type();
            resp.set_body("<html><body><h1>404 Not Found</h1></body></html>");

            LOG_WARN("Not found: " << path);
        }
    });

    std::cout << "Static File Server starting on http://0.0.0.0:8080" << std::endl;
    std::cout << "Serving files from: " << fs::absolute(root_dir) << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    server.start();

    return 0;
}
