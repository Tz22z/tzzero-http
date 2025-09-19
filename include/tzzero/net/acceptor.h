#pragma once

#include <functional>
#include <string>
#include <cstdint>

namespace tzzero::core {
class EventLoop;
}

namespace tzzero::net {

class Acceptor {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const std::string& peer_addr)>;

    Acceptor(core::EventLoop* loop, const std::string& listen_addr, uint16_t port);
    ~Acceptor();

    // 不可拷贝
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    void set_new_connection_callback(const NewConnectionCallback& cb) {
        new_connection_callback_ = cb;
    }

    bool listening() const { return listening_; }
    void listen();

private:
    void handle_read();
    int create_nonblocking_socket();
    void bind_and_listen();

    core::EventLoop* loop_;
    const std::string listen_addr_;
    uint16_t port_;
    int accept_fd_;
    int idle_fd_;  // 保留文件描述符用于 EMFILE 保护
    bool listening_;
    NewConnectionCallback new_connection_callback_;

    static constexpr int kMaxAcceptPerLoop = 10000;
};

}  // namespace tzzero::net