#include "tzzero/http/http_parser.h"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace tzzero::http {

HttpParser::HttpParser()
    : has_error_(false)
    , content_length_(0)
    , expect_body_(false)
{
}

bool HttpParser::parse_request(utils::Buffer& buffer, HttpRequest& request) {
    while (true) {
        // 解析请求行
        if (request.get_parse_state() == HttpRequest::PARSE_REQUEST_LINE) {
            const char* crlf = buffer.find_crlf();
            if (!crlf) {
                break; // 需要更多数据
            }

            std::string line(buffer.peek(), crlf - buffer.peek());
            buffer.retrieve(crlf - buffer.peek() + 2); // +2 for \r\n

            if (!parse_request_line(line, request)) {
                has_error_ = true;
                return false;
            }

            request.set_parse_state(HttpRequest::PARSE_HEADERS);
        }
        // 解析请求头
        else if (request.get_parse_state() == HttpRequest::PARSE_HEADERS) {
            const char* crlf = buffer.find_crlf();
            if (!crlf) {
                break; // 需要更多数据
            }

            std::string line(buffer.peek(), crlf - buffer.peek());
            buffer.retrieve(crlf - buffer.peek() + 2); // +2 for \r\n

            if (line.empty()) {
                // 空行表示头部结束
                content_length_ = request.get_content_length();
                if (content_length_ > 0) {
                    expect_body_ = true;
                    request.set_parse_state(HttpRequest::PARSE_BODY);
                } else {
                    request.set_parse_state(HttpRequest::PARSE_COMPLETE);
                    return true; // 完整请求
                }
            } else {
                if (!parse_header_line(line, request)) {
                    has_error_ = true;
                    return false;
                }
            }
        }
        // 解析请求体
        else if (request.get_parse_state() == HttpRequest::PARSE_BODY) {
            if (buffer.readable_bytes() >= content_length_) {
                // 已有完整请求体
                std::string body = buffer.retrieve_as_string(content_length_);
                request.set_body(std::move(body));
                request.set_parse_state(HttpRequest::PARSE_COMPLETE);
                return true; // 完整请求
            } else {
                break; // 需要更多数据
            }
        }
        // 解析完成
        else if (request.get_parse_state() == HttpRequest::PARSE_COMPLETE) {
            return true;
        }
        // 错误状态
        else {
            has_error_ = true;
            return false;
        }
    }

    return false; // 请求不完整
}

void HttpParser::reset() {
    has_error_ = false;
    content_length_ = 0;
    expect_body_ = false;
}

bool HttpParser::parse_request_line(const std::string& line, HttpRequest& request) {
    std::istringstream iss(line);
    std::string method_str, path_and_query, version_str;

    if (!(iss >> method_str >> path_and_query >> version_str)) {
        return false;
    }

    // 解析方法
    HttpMethod method = string_to_method(method_str);
    if (method == HttpMethod::INVALID) {
        return false;
    }
    request.set_method(method);

    // 解析路径和查询参数
    size_t query_pos = path_and_query.find('?');
    if (query_pos != std::string::npos) {
        request.set_path(path_and_query.substr(0, query_pos));
        request.set_query(path_and_query.substr(query_pos + 1));
    } else {
        request.set_path(path_and_query);
    }

    // 解析版本
    HttpVersion version = string_to_version(version_str);
    if (version == HttpVersion::UNKNOWN) {
        return false;
    }
    request.set_version(version);

    return true;
}

bool HttpParser::parse_header_line(const std::string& line, HttpRequest& request) {
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }

    std::string field = line.substr(0, colon_pos);
    std::string value = line.substr(colon_pos + 1);

    // 去除首尾空白
    field.erase(field.find_last_not_of(" \t") + 1);
    field.erase(0, field.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);
    value.erase(0, value.find_first_not_of(" \t"));

    if (field.empty()) {
        return false;
    }

    request.add_header(field, value);
    return true;
}

HttpMethod HttpParser::string_to_method(const std::string& method_str) {
    if (method_str == "GET") return HttpMethod::GET;
    if (method_str == "POST") return HttpMethod::POST;
    if (method_str == "PUT") return HttpMethod::PUT;
    if (method_str == "DELETE") return HttpMethod::DELETE;
    if (method_str == "HEAD") return HttpMethod::HEAD;
    if (method_str == "OPTIONS") return HttpMethod::OPTIONS;
    if (method_str == "PATCH") return HttpMethod::PATCH;
    if (method_str == "CONNECT") return HttpMethod::CONNECT;
    if (method_str == "TRACE") return HttpMethod::TRACE;
    return HttpMethod::INVALID;
}

HttpVersion HttpParser::string_to_version(const std::string& version_str) {
    if (version_str == "HTTP/1.0") return HttpVersion::HTTP_1_0;
    if (version_str == "HTTP/1.1") return HttpVersion::HTTP_1_1;
    if (version_str == "HTTP/2.0") return HttpVersion::HTTP_2_0;
    return HttpVersion::UNKNOWN;
}

} // namespace tzzero::http
