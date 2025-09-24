#include "tzzero/http/http_request.h"
#include <algorithm>
#include <sstream>

namespace tzzero::http {

// 获取 HTTP 方法的字符串表示
std::string HttpRequest::get_method_string() const {
    switch (method_) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::CONNECT: return "CONNECT";
        case HttpMethod::TRACE: return "TRACE";
        default: return "UNKNOWN";
    }
}

// 获取 HTTP 版本的字符串表示
std::string HttpRequest::get_version_string() const {
    switch (version_) {
        case HttpVersion::HTTP_1_0: return "HTTP/1.0";
        case HttpVersion::HTTP_1_1: return "HTTP/1.1";
        case HttpVersion::HTTP_2_0: return "HTTP/2.0";
        default: return "HTTP/1.1";
    }
}

// 添加头部字段（如果已存在则追加）
void HttpRequest::add_header(const std::string& field, const std::string& value) {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    
    auto it = headers_.find(lower_field);
    if (it != headers_.end()) {
        it->second += ", " + value;
    } else {
        headers_[lower_field] = value;
    }
}

// 设置头部字段（覆盖已存在的）
void HttpRequest::set_header(const std::string& field, const std::string& value) {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    headers_[lower_field] = value;
}

// 获取头部字段值
std::string HttpRequest::get_header(const std::string& field) const {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    
    auto it = headers_.find(lower_field);
    return it != headers_.end() ? it->second : "";
}

// 检查是否存在指定头部字段
bool HttpRequest::has_header(const std::string& field) const {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    return headers_.find(lower_field) != headers_.end();
}

// 移除头部字段
void HttpRequest::remove_header(const std::string& field) {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    headers_.erase(lower_field);
}

// 获取内容长度
size_t HttpRequest::get_content_length() const {
    std::string content_length = get_header("content-length");
    if (content_length.empty()) {
        return 0;
    }
    
    try {
        return std::stoull(content_length);
    } catch (const std::exception&) {
        return 0;
    }
}

// 检查是否支持 Keep-Alive 连接
bool HttpRequest::keep_alive() const {
    std::string connection = get_header("connection");
    std::transform(connection.begin(), connection.end(), connection.begin(), ::tolower);
    
    if (version_ == HttpVersion::HTTP_1_1) {
        // HTTP/1.1 默认启用 keep-alive，除非明确指定 close
        return connection != "close";
    } else {
        // HTTP/1.0 默认 close，除非明确指定 keep-alive
        return connection == "keep-alive";
    }
}

// 重置请求对象以便复用
void HttpRequest::reset() {
    method_ = HttpMethod::INVALID;
    path_.clear();
    query_.clear();
    version_ = HttpVersion::UNKNOWN;
    headers_.clear();
    body_.clear();
    parse_state_ = PARSE_REQUEST_LINE;
    stream_id_ = 0;
}

// 将请求转换为字符串表示
std::string HttpRequest::to_string() const {
    std::ostringstream oss;
    
    // 请求行
    oss << get_method_string() << " " << path_;
    if (!query_.empty()) {
        oss << "?" << query_;
    }
    oss << " " << get_version_string() << "\r\n";
    
    // 头部字段
    for (const auto& header : headers_) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    oss << "\r\n";
    
    // 请求体
    if (!body_.empty()) {
        oss << body_;
    }
    
    return oss.str();
}

} // namespace tzzero::http