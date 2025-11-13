#include "tzzero/http/http_server.h"
#include "tzzero/http/http_parser.h"
#include "tzzero/core/event_loop.h"
#include "tzzero/utils/logger.h"
#include <unordered_map>

namespace tzzero::http {

HttpServer::HttpServer(core::EventLoop* loop, const std::string& listen_addr,
                       uint16_t port, const std::string& name)
    : loop_(loop)
    , server_(std::make_unique<net::TcpServer>(loop, listen_addr, port, name))
{
    server_->set_connection_callback([this](const net::TcpConnectionPtr& conn) {
        on_connection(conn);
    });

    server_->set_message_callback([this](const net::TcpConnectionPtr& conn, utils::Buffer& buffer) {
        on_message(conn, buffer);
    });
}

HttpServer::~HttpServer() = default;

void HttpServer::start() {
    server_->start();
}

void HttpServer::stop() {
    server_->stop();
}

void HttpServer::set_thread_num(int num_threads) {
    server_->set_thread_num(num_threads);
}

#ifdef ENABLE_TLS
void HttpServer::enable_tls(const std::string& cert_file, const std::string& key_file) {
    tls_enabled_ = true;
    cert_file_ = cert_file;
    key_file_ = key_file;
    LOG_INFO("TLS enabled with cert: " << cert_file << ", key: " << key_file);
}
#endif

void HttpServer::on_connection(const net::TcpConnectionPtr& conn) {
    LOG_INFO("HttpServer - " << conn->get_local_address()
             << " -> " << conn->get_peer_address() << " is "
             << (conn->connected() ? "UP" : "DOWN"));

    if (conn->connected()) {
        // 为此连接创建HTTP解析器
        auto parser = std::make_shared<HttpParser>();
        conn->set_context(parser);

        // 设置TCP选项
        conn->set_tcp_no_delay(true);
        conn->set_keep_alive(true);
    }
}

void HttpServer::on_message(const net::TcpConnectionPtr& conn, utils::Buffer& buffer) {
    std::shared_ptr<HttpParser> parser;
    try {
        parser = std::any_cast<std::shared_ptr<HttpParser>>(conn->get_context());
    } catch (const std::bad_any_cast&) {
        // 上下文未设置或类型错误，创建新解析器
        parser = std::make_shared<HttpParser>();
        conn->set_context(parser);
    }

    HttpRequest request;
    if (parser->parse_request(buffer, request)) {
        // 收到完整请求
        on_request(conn, request);

        // 重置解析器用于下一个请求（Keep-Alive）
        parser->reset();
    } else if (parser->has_error()) {
        // 解析错误，关闭连接
        LOG_ERROR("HTTP parse error from " << conn->get_peer_address());
        conn->shutdown();
        return;
    }
    // 如果请求不完整，等待更多数据
}

void HttpServer::on_request(const net::TcpConnectionPtr& conn, const HttpRequest& req) {
    HttpResponse response;

    // 设置默认头部
    response.set_header("Server", "TZZeroHTTP/1.0");

    // 处理Keep-Alive
    bool close_connection = !req.keep_alive() || !keep_alive_enabled_;
    response.set_close_connection(close_connection);

    if (close_connection) {
        response.set_header("Connection", "close");
    } else {
        response.set_header("Connection", "keep-alive");
        if (keep_alive_timeout_ > 0) {
            response.set_header("Keep-Alive", "timeout=" + std::to_string(keep_alive_timeout_));
        }
    }

    // 调用用户回调
    if (http_callback_) {
        http_callback_(req, response);
    } else {
        // 默认404响应
        response.set_status_code(HttpStatusCode::NOT_FOUND);
        response.set_html_content_type();
        response.set_body("<html><body><h1>404 Not Found</h1></body></html>");
    }

    // 发送响应
    std::string response_data;
    response.append_to_buffer(response_data);
    conn->send(response_data);

    if (response.close_connection()) {
        conn->shutdown();
    }
}

} // namespace tzzero::http
