#pragma once

#include "tzzero/net/tcp_server.h"
#include "tzzero/http/http_request.h"
#include "tzzero/http/http_response.h"
#include <functional>
#include <memory>

namespace tzzero::http {

/**
 * HTTP服务器
 * 封装TCP服务器，提供HTTP协议处理
 */
class HttpServer {
public:
    using HttpCallback = std::function<void(const HttpRequest&, HttpResponse&)>;

    HttpServer(core::EventLoop* loop, const std::string& listen_addr, uint16_t port,
               const std::string& name = "TZZeroHTTP");
    ~HttpServer();

    // 禁止拷贝
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    /**
     * 启动/停止服务器
     */
    void start();
    void stop();

    /**
     * 设置HTTP请求处理回调
     */
    void set_http_callback(const HttpCallback& cb) { http_callback_ = cb; }

    /**
     * 服务器配置
     */
    void set_thread_num(int num_threads);
    void enable_keep_alive(bool enable) { keep_alive_enabled_ = enable; }
    void set_keep_alive_timeout(int seconds) { keep_alive_timeout_ = seconds; }

    /**
     * HTTP/2配置（预留接口）
     */
    void enable_http2(bool enable) { http2_enabled_ = enable; }

    /**
     * TLS配置（预留接口）
     */
#ifdef ENABLE_TLS
    void enable_tls(const std::string& cert_file, const std::string& key_file);
#endif

private:
    // 连接建立/关闭回调
    void on_connection(const net::TcpConnectionPtr& conn);

    // 消息到达回调
    void on_message(const net::TcpConnectionPtr& conn, utils::Buffer& buffer);

    // 完整HTTP请求到达回调
    void on_request(const net::TcpConnectionPtr& conn, const HttpRequest& req);

    core::EventLoop* loop_;                      // 事件循环
    std::unique_ptr<net::TcpServer> server_;     // TCP服务器
    HttpCallback http_callback_;                 // HTTP请求处理回调

    bool keep_alive_enabled_{true};              // 是否启用Keep-Alive
    int keep_alive_timeout_{60};                 // Keep-Alive超时（秒）
    bool http2_enabled_{false};                  // 是否启用HTTP/2

#ifdef ENABLE_TLS
    bool tls_enabled_{false};
    std::string cert_file_;
    std::string key_file_;
#endif
};

} // namespace tzzero::http
