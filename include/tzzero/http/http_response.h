#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace tzzero::http {

enum class HttpStatusCode {
    // 1xx Informational
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    
    // 2xx Success
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    PARTIAL_CONTENT = 206,
    
    // 3xx Redirection
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    NOT_MODIFIED = 304,
    TEMPORARY_REDIRECT = 307,
    
    // 4xx Client Error
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    REQUEST_TIMEOUT = 408,
    LENGTH_REQUIRED = 411,
    PAYLOAD_TOO_LARGE = 413,
    
    // 5xx Server Error
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505
};

class HttpResponse {
public:
    HttpResponse() = default;
    ~HttpResponse() = default;

    // Copyable and movable
    HttpResponse(const HttpResponse&) = default;
    HttpResponse& operator=(const HttpResponse&) = default;
    HttpResponse(HttpResponse&&) = default;
    HttpResponse& operator=(HttpResponse&&) = default;

    // Status
    void set_status_code(HttpStatusCode code) { status_code_ = code; }
    HttpStatusCode get_status_code() const { return status_code_; }
    std::string get_status_message() const;

    void set_close_connection(bool close) { close_connection_ = close; }
    bool close_connection() const { return close_connection_; }

    // Headers
    void add_header(const std::string& field, const std::string& value);
    void set_header(const std::string& field, const std::string& value);
    std::string get_header(const std::string& field) const;
    bool has_header(const std::string& field) const;
    void remove_header(const std::string& field);
    const std::unordered_map<std::string, std::string>& get_headers() const { return headers_; }

    // Body
    void set_body(const std::string& body);
    void set_body(std::string&& body);
    void append_body(const std::string& data);
    const std::string& get_body() const { return body_; }
    void clear_body() { body_.clear(); }

    // Content type helpers
    void set_content_type(const std::string& content_type);
    void set_json_content_type() { set_content_type("application/json; charset=utf-8"); }
    void set_html_content_type() { set_content_type("text/html; charset=utf-8"); }
    void set_text_content_type() { set_content_type("text/plain; charset=utf-8"); }

    // Redirect
    void redirect(const std::string& url, HttpStatusCode code = HttpStatusCode::FOUND);

    // Reset for reuse
    void reset();

    // Serialization
    std::string to_buffer() const;
    void append_to_buffer(std::string& buffer) const;

    // HTTP/2 specific
    void set_stream_id(uint32_t stream_id) { stream_id_ = stream_id; }
    uint32_t get_stream_id() const { return stream_id_; }

private:
    void ensure_content_length();
    
    HttpStatusCode status_code_{HttpStatusCode::OK};
    bool close_connection_{false};
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    uint32_t stream_id_{0}; // For HTTP/2
};

using HttpResponsePtr = std::shared_ptr<HttpResponse>;

} // namespace tzzero::http
