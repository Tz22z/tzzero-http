/*
 * RESTful API 示例
 * 演示如何实现一个简单的用户管理 API
 */

#include "tzzero/http/http_server.h"
#include "tzzero/http/http_request.h"
#include "tzzero/http/http_response.h"
#include "tzzero/utils/logger.h"
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>

using namespace tzzero::http;
using namespace tzzero::utils;

// 简单的内存数据库
struct User {
    int id;
    std::string name;
    std::string email;
};

class UserDatabase {
public:
    void add_user(int id, const std::string& name, const std::string& email) {
        std::lock_guard<std::mutex> lock(mutex_);
        users_[id] = {id, name, email};
    }

    bool get_user(int id, User& user) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = users_.find(id);
        if (it != users_.end()) {
            user = it->second;
            return true;
        }
        return false;
    }

    std::string get_all_users_json() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss;
        oss << "[";
        bool first = true;
        for (const auto& [id, user] : users_) {
            if (!first) oss << ",";
            oss << "{\"id\":" << user.id
                << ",\"name\":\"" << user.name << "\""
                << ",\"email\":\"" << user.email << "\"}";
            first = false;
        }
        oss << "]";
        return oss.str();
    }

    bool delete_user(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        return users_.erase(id) > 0;
    }

private:
    std::map<int, User> users_;
    std::mutex mutex_;
};

int main() {
    Logger::instance().set_level(LogLevel::INFO);

    // 创建数据库和服务器
    UserDatabase db;
    HttpServer server("0.0.0.0", 8080);

    // 添加一些测试数据
    db.add_user(1, "Alice", "alice@example.com");
    db.add_user(2, "Bob", "bob@example.com");

    // GET /api/users - 获取所有用户
    server.route("/api/users", [&db](const HttpRequest& req, HttpResponse& resp) {
        if (req.get_method() == HttpMethod::GET) {
            resp.set_status_code(HttpStatusCode::OK);
            resp.set_json_content_type();
            resp.set_body(db.get_all_users_json());
        } else {
            resp.set_status_code(HttpStatusCode::METHOD_NOT_ALLOWED);
        }
    });

    // GET /api/user/{id} - 获取特定用户
    server.route_pattern("/api/user/", [&db](const HttpRequest& req, HttpResponse& resp) {
        if (req.get_method() == HttpMethod::GET) {
            // 从路径中提取 ID
            std::string path = req.get_path();
            int id = std::stoi(path.substr(path.find_last_of('/') + 1));

            User user;
            if (db.get_user(id, user)) {
                std::ostringstream json;
                json << "{\"id\":" << user.id
                     << ",\"name\":\"" << user.name << "\""
                     << ",\"email\":\"" << user.email << "\"}";
                resp.set_status_code(HttpStatusCode::OK);
                resp.set_json_content_type();
                resp.set_body(json.str());
            } else {
                resp.set_status_code(HttpStatusCode::NOT_FOUND);
                resp.set_json_content_type();
                resp.set_body("{\"error\":\"User not found\"}");
            }
        } else if (req.get_method() == HttpMethod::DELETE) {
            std::string path = req.get_path();
            int id = std::stoi(path.substr(path.find_last_of('/') + 1));

            if (db.delete_user(id)) {
                resp.set_status_code(HttpStatusCode::NO_CONTENT);
            } else {
                resp.set_status_code(HttpStatusCode::NOT_FOUND);
            }
        } else {
            resp.set_status_code(HttpStatusCode::METHOD_NOT_ALLOWED);
        }
    });

    std::cout << "RESTful API Server starting on http://0.0.0.0:8080" << std::endl;
    std::cout << "Try these commands:" << std::endl;
    std::cout << "  curl http://localhost:8080/api/users" << std::endl;
    std::cout << "  curl http://localhost:8080/api/user/1" << std::endl;
    std::cout << "  curl -X DELETE http://localhost:8080/api/user/1" << std::endl;

    server.start();

    return 0;
}
