#pragma once

#include "tzzero/http/http_request.h"
#include "tzzero/utils/buffer.h"
#include <functional>

namespace tzzero::http {

/**
 * HTTP请求解析器
 * 实现流式解析，支持分片数据
 */
class HttpParser {
public:
    using RequestCallback = std::function<void(const HttpRequest&)>;

    HttpParser();
    ~HttpParser() = default;

    // 禁止拷贝
    HttpParser(const HttpParser&) = delete;
    HttpParser& operator=(const HttpParser&) = delete;

    /**
     * 从缓冲区解析HTTP请求
     * @param buffer 输入缓冲区
     * @param request 解析结果
     * @return true表示解析出完整请求，false表示需要更多数据
     */
    bool parse_request(utils::Buffer& buffer, HttpRequest& request);

    /**
     * 设置请求完成回调
     */
    void set_request_callback(const RequestCallback& cb) { request_callback_ = cb; }

    /**
     * 重置解析器状态
     */
    void reset();

    /**
     * 检查是否有错误
     */
    bool has_error() const { return has_error_; }

private:
    // 解析请求行：GET /path HTTP/1.1
    bool parse_request_line(const std::string& line, HttpRequest& request);

    // 解析头部行：Header: Value
    bool parse_header_line(const std::string& line, HttpRequest& request);

    // 字符串转HTTP方法
    HttpMethod string_to_method(const std::string& method_str);

    // 字符串转HTTP版本
    HttpVersion string_to_version(const std::string& version_str);

    RequestCallback request_callback_;  // 请求完成回调
    bool has_error_;                    // 错误标志
    size_t content_length_;             // 请求体长度
    bool expect_body_;                  // 是否期待请求体
};

} // namespace tzzero::http
