#pragma once

#include "tzzero/net/tcp_connection.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <atomic>

namespace tzzero::core {
class EventLoop;
}

namespace tzzero::net {

class Acceptor;
class EventLoopThreadPool;

/**
 * TCP服务器
 * 管理连接的生命周期，支持多线程事件循环
 */
class TcpServer {
public:
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, utils::Buffer&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

    TcpServer(core::EventLoop* loop, const std::string& listen_addr,
              uint16_t port, const std::string& name);
    ~TcpServer();

    // 禁止拷贝
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    /**
     * 启动服务器
     */
    void start();

    /**
     * 停止服务器
     */
    void stop();

    /**
     * 设置工作线程数（必须在start之前调用）
     */
    void set_thread_num(int num_threads);

    const std::string& get_name() const { return name_; }
    const std::string& get_ip_port() const { return ip_port_; }

    /**
     * 设置回调函数
     */
    void set_connection_callback(const ConnectionCallback& cb) {
        connection_callback_ = cb;
    }

    void set_message_callback(const MessageCallback& cb) {
        message_callback_ = cb;
    }

    void set_write_complete_callback(const WriteCompleteCallback& cb) {
        write_complete_callback_ = cb;
    }

private:
    // 新连接到达
    void new_connection(int sockfd, const std::string& peer_addr);

    // 移除连接
    void remove_connection(const TcpConnectionPtr& conn);
    void remove_connection_in_loop(const TcpConnectionPtr& conn);

    core::EventLoop* loop_;                      // 主EventLoop
    const std::string ip_port_;                  // 监听地址:端口
    const std::string name_;                     // 服务器名称
    std::unique_ptr<Acceptor> acceptor_;         // 连接接受器
    std::unique_ptr<EventLoopThreadPool> thread_pool_;  // 工作线程池

    ConnectionCallback connection_callback_;      // 新连接回调
    MessageCallback message_callback_;            // 消息到达回调
    WriteCompleteCallback write_complete_callback_;  // 写完成回调

    std::atomic<bool> started_;                  // 是否已启动
    int next_conn_id_;                           // 下一个连接ID
    std::unordered_map<std::string, TcpConnectionPtr> connections_;  // 连接映射
};

} // namespace tzzero::net
