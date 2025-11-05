#pragma once

#include "tzzero/utils/buffer.h"

#include <memory>
#include <functional>
#include <string>
#include <atomic>
#include <any>

namespace tzzero::core {
class EventLoop;
}

namespace tzzero::net {

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, tzzero::utils::Buffer&)>;    
    using CloseCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using WriteCompleteCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using HighWaterMarkCallback = std::function<void(const std::shared_ptr<TcpConnection>&, size_t)>;

    enum State {
        CONNECTING,
        CONNECTED,
        DISCONNECTING,
        DISCONNECTED
    };

    TcpConnection(core::EventLoop* loop, const std::string& name, int sockfd);
    ~TcpConnection();

    // 不可拷贝
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    // 连接状态
    State get_state() const { return state_; }
    bool connected() const { return state_ == CONNECTED; }
    bool disconnected() const { return state_ == DISCONNECTED; }

    // 连接信息
    const std::string& get_name() const { return name_; }
    int get_fd() const { return socket_fd_; }
    core::EventLoop* get_loop() const { return loop_; }
    const std::string& get_local_address() const { return local_addr_; }
    const std::string& get_peer_address() const { return peer_addr_; }

    // 输入输出操作
    void send(const void* data, size_t len);
    void send(const std::string& message);
    void send(tzzero::utils::Buffer& buffer);
    void shutdown();
    void force_close();

    // 回调函数
    void set_message_callback(const MessageCallback& cb) { message_callback_ = cb; }
    void set_close_callback(const CloseCallback& cb) { close_callback_ = cb; }
    void set_write_complete_callback(const WriteCompleteCallback& cb) { write_complete_callback_ = cb; }
    void set_high_water_mark_callback(const HighWaterMarkCallback& cb, size_t high_water_mark);
    
    // 连接管理
    void connection_established();
    void connection_destroyed();

    // 缓冲区管理
    tzzero::utils::Buffer& get_input_buffer() { return input_buffer_; }
    tzzero::utils::Buffer& get_output_buffer() { return output_buffer_; }

    // TCP 选项
    void set_tcp_no_delay(bool on);
    void set_keep_alive(bool on);

    // 用于存储连接特定数据的上下文
    void set_context(const std::any& context) { context_ = context; }
    const std::any& get_context() const { return context_; }
    std::any& get_mutable_context() { return context_; }

private:
    void handle_read();
    void handle_write();
    void handle_close();
    void handle_error();

    void send_in_loop(const void* data, size_t len);
    void shutdown_in_loop();
    void force_close_in_loop();

    core::EventLoop* loop_;
    const std::string name_;
    State state_;
    int socket_fd_;
    std::string local_addr_;
    std::string peer_addr_;

    tzzero::utils::Buffer input_buffer_;
    tzzero::utils::Buffer output_buffer_;
    size_t high_water_mark_;

    MessageCallback message_callback_;
    CloseCallback close_callback_;
    WriteCompleteCallback write_complete_callback_;
    HighWaterMarkCallback high_water_mark_callback_;

    std::any context_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

}  // namespace tzzero::net
