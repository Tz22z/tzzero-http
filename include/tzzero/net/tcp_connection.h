#pragma once

#include "tzzerohttp/utils/buffer.h
#include <memory>
#include <functional>
#include <string>
#include <atomic>
#include <any>

namespace tzzerohttp::core {
class EventLoop;
}

namespace tzzero::net {

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, utils::Buffer&)>;    
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

    // Non-copyable
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    // Connection state
    State get_state() const { return state_; }
    bool connected() const { return state_ == CONNECTED; }
    bool disconnected() const { return state_ == DISCONNECTED; }

    // Connection info
    const std::string& get_name() const { return name_; }
    int get_fd() const { return socket_fd_; }
    core::EventLoop* get_loop() const { return loop_; }
    const std::string& get_local_address() const { return local_addr_; }
    const std::string& get_peer_address() const { return peer_addr_; }

    // I/O operations
    void send(const void* data, size_t len);
    void send(const std::string& message);
    void send(utils::Buffer& buffer);
    void shutdown();
    void force_close();

    // Callbacks
    void set_message_callback(const MessageCallback& cb) { message_callback_ = cb; }
    void set_close_callback(const CloseCallback& cb) { close_callback_ = cb; }
    void set_write_complete_callback(const WriteCompleteCallback& cb) { write_complete_callback_ = cb; }
    void set_high_water_mark_callback(const HighWaterMarkCallback& cb, size_t high_water_mark);
    
    // Connection management
    void connection_established();
    void connection_destroyed();

    // Buffer management
    utils::Buffer& get_input_buffer() { return input_buffer_; }
    utils::Buffer& get_output_buffer() { return output_buffer_; }

    // TCP options
    void set_tcp_no_delay(bool on);
    void set_keep_alive(bool on);

    // Context for storing connection-specific data
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
    int socket_fd_;

    int socket_fd_;
    std::string local_addr_;
    std::string peer_addr_;

    utils::Buffer input_buffer_;
    utils::Buffer output_buffer_;
    size_t high_water_mark_;

    MessageCallback message_callback_;
    CloseCallback close_callback_;
    WriteCompleteCallback write_complete_callback_;
    HighWaterMarkCallback high_water_mark_callback_;

    std::any context_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

} // namespace tzzero::net
