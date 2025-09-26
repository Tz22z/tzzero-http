#include "tzzero/http/http_response.h"
#include <algorithm>
#include <sstream>
#include <ctime>
#include <iomanip>

namespace tzzero::http {

std::string HttpResponse::get_status_message() const {
    switch (status_code_) {
        case HttpStatusCode::CONTINUE: return "Continue";
        case HttpStatusCode::SWITCHING_PROTOCOLS: return "Switching Protocols";
        case HttpStatusCode::OK: return "OK";
        case HttpStatusCode::CREATED: return "Created";
        case HttpStatusCode::ACCEPTED: return "Accepted";
        case HttpStatusCode::NO_CONTENT: return "No Content";
        case HttpStatusCode::PARTIAL_CONTENT: return "Partial Content";
        case HttpStatusCode::MOVED_PERMANENTLY: return "Moved Permanently";
        case HttpStatusCode::FOUND: return "Found";
        case HttpStatusCode::NOT_MODIFIED: return "Not Modified";
        case HttpStatusCode::TEMPORARY_REDIRECT: return "Temporary Redirect";
        case HttpStatusCode::BAD_REQUEST: return "Bad Request";
        case HttpStatusCode::UNAUTHORIZED: return "Unauthorized";
        case HttpStatusCode::FORBIDDEN: return "Forbidden";
        case HttpStatusCode::NOT_FOUND: return "Not Found";
        case HttpStatusCode::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HttpStatusCode::REQUEST_TIMEOUT: return "Request Timeout";
        case HttpStatusCode::LENGTH_REQUIRED: return "Length Required";
        case HttpStatusCode::PAYLOAD_TOO_LARGE: return "Payload Too Large";
        case HttpStatusCode::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatusCode::NOT_IMPLEMENTED: return "Not Implemented";
        case HttpStatusCode::BAD_GATEWAY: return "Bad Gateway";
        case HttpStatusCode::SERVICE_UNAVAILABLE: return "Service Unavailable";
        case HttpStatusCode::GATEWAY_TIMEOUT: return "Gateway Timeout";
        case HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
        default: return "Unknown";
    }
}

void HttpResponse::add_header(const std::string& field, const std::string& value) {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    
    auto it = headers_.find(lower_field);
    if (it != headers_.end()) {
        it->second += ", " + value;
    } else {
        headers_[lower_field] = value;
    }
}

void HttpResponse::set_header(const std::string& field, const std::string& value) {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    headers_[lower_field] = value;
}

std::string HttpResponse::get_header(const std::string& field) const {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    
    auto it = headers_.find(lower_field);
    return it != headers_.end() ? it->second : "";
}

bool HttpResponse::has_header(const std::string& field) const {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    return headers_.find(lower_field) != headers_.end();
}

void HttpResponse::remove_header(const std::string& field) {
    std::string lower_field = field;
    std::transform(lower_field.begin(), lower_field.end(), lower_field.begin(), ::tolower);
    headers_.erase(lower_field);
}

void HttpResponse::set_body(const std::string& body) {
    body_ = body;
    ensure_content_length();
}

void HttpResponse::set_body(std::string&& body) {
    body_ = std::move(body);
    ensure_content_length();
}

void HttpResponse::append_body(const std::string& data) {
    body_ += data;
    ensure_content_length();
}

void HttpResponse::set_content_type(const std::string& content_type) {
    set_header("content-type", content_type);
}

void HttpResponse::redirect(const std::string& url, HttpStatusCode code) {
    set_status_code(code);
    set_header("location", url);
    set_html_content_type();
    set_body("<html><body><h1>Redirecting...</h1><p>Please follow <a href=\"" + url + "\">this link</a>.</p></body></html>");
}

void HttpResponse::reset() {
    status_code_ = HttpStatusCode::OK;
    close_connection_ = false;
    headers_.clear();
    body_.clear();
    stream_id_ = 0;
}

std::string HttpResponse::to_buffer() const {
    std::string buffer;
    append_to_buffer(buffer);
    return buffer;
}

void HttpResponse::append_to_buffer(std::string& buffer) const {
    // Status line
    buffer += "HTTP/1.1 ";
    buffer += std::to_string(static_cast<int>(status_code_));
    buffer += " ";
    buffer += get_status_message();
    buffer += "\r\n";
    
    // Headers
    for (const auto& header : headers_) {
        buffer += header.first;
        buffer += ": ";
        buffer += header.second;
        buffer += "\r\n";
    }
    
    // Connection header
    if (close_connection_) {
        buffer += "connection: close\r\n";
    } else {
        buffer += "connection: keep-alive\r\n";
    }
    
    // Server header
    if (!has_header("server")) {
        buffer += "server: TZZeroHTTP/1.0\r\n";
    }
    
    // Date header
    if (!has_header("date")) {
        auto now = std::time(nullptr);
        auto tm = *::gmtime(&now);
        char date_buf[100];
        ::strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
        buffer += "date: ";
        buffer += date_buf;
        buffer += "\r\n";
    }
    
    buffer += "\r\n";
    
    // Body
    if (!body_.empty()) {
        buffer += body_;
    }
}

void HttpResponse::ensure_content_length() {
    if (!body_.empty() && !has_header("content-length")) {
        set_header("content-length", std::to_string(body_.size()));
    }
}

} // namespace tzzero::http
