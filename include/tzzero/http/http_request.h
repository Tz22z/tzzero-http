#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace tzzero::http {

// HTTP 请求方法枚举
enum class HttpMethod {
    INVALID,    // 无效方法
    GET,        // 获取资源
    POST,       // 提交数据
    PUT,        // 更新资源
    DELETE,     // 删除资源
    HEAD,       // 获取头部信息
    OPTIONS,    // 选项查询
    PATCH,      // 部分更新
    CONNECT,    // 建立隧道
    TRACE       // 路径追踪
};

// HTTP 版本枚举
enum class HttpVersion {
    UNKNOWN,    // 未知版本
    HTTP_1_0,   // HTTP/1.0
    HTTP_1_1,   // HTTP/1.1
    HTTP_2_0    // HTTP/2.0
};

// HTTP 请求类
class HttpRequest {
public:
    HttpRequest() = default;
    ~HttpRequest() = default;

    // 可拷贝和可移动
    HttpRequest(const HttpRequest&) = default;
    HttpRequest& operator=(const HttpRequest&) = default;
    HttpRequest(HttpRequest&&) = default;
    HttpRequest& operator=(HttpRequest&&) = default;

    // 请求行相关方法
    void set_method(HttpMethod method) { method_ = method; }
    HttpMethod get_method() const { return method_; }
    std::string get_method_string() const;

    void set_path(const std::string& path) { path_ = path; }
    const std::string& get_path() const { return path_; }

    void set_query(const std::string& query) { query_ = query; }
    const std::string& get_query() const { return query_; }

    void set_version(HttpVersion version) { version_ = version; }
    HttpVersion get_version() const { return version_; }
    std::string get_version_string() const;

    // 头部字段相关方法
    void add_header(const std::string& field, const std::string& value);
    void set_header(const std::string& field, const std::string& value);
    std::string get_header(const std::string& field) const;
    bool has_header(const std::string& field) const;
    void remove_header(const std::string& field);
    const std::unordered_map<std::string, std::string>& get_headers() const { return headers_; }

    // 请求体相关方法
    void set_body(const std::string& body) { body_ = body; }
    void set_body(std::string&& body) { body_ = std::move(body); }
    const std::string& get_body() const { return body_; }
    size_t get_content_length() const;

    // 连接管理
    bool keep_alive() const;

    // 解析状态枚举
    enum ParseState {
        PARSE_REQUEST_LINE,     // 解析请求行
        PARSE_HEADERS,          // 解析头部字段
        PARSE_BODY,             // 解析请求体
        PARSE_COMPLETE,         // 解析完成
        PARSE_ERROR             // 解析错误
    };

    void set_parse_state(ParseState state) { parse_state_ = state; }
    ParseState get_parse_state() const { return parse_state_; }

    // 重置以便复用
    void reset();

    // 字符串表示
    std::string to_string() const;

    // HTTP/2 专用功能
    void set_stream_id(uint32_t stream_id) { stream_id_ = stream_id; }
    uint32_t get_stream_id() const { return stream_id_; }

private:
    HttpMethod method_{HttpMethod::INVALID};    // HTTP 方法
    std::string path_;                          // 请求路径
    std::string query_;                         // 查询参数
    HttpVersion version_{HttpVersion::UNKNOWN}; // HTTP 版本

    std::unordered_map<std::string, std::string> headers_;  // 头部字段
    std::string body_;                                      // 请求体

    ParseState parse_state_{PARSE_REQUEST_LINE};    // 解析状态
    uint32_t stream_id_{0};                         // HTTP/2 流ID
};

using HttpRequestPtr = std::shared_ptr<HttpRequest>;

} // namespace tzzero::http