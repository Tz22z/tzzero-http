#include "tzzero/net/acceptor.h"
#include "tzzero/core/event_loop.h"
#include "tzzero/core/poller.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace tzzero::net {

Acceptor::Acceptor(core::EventLoop* loop, const std::string& listen_addr, uint16_t port)
    : loop_(loop)
    , accept_fd_(create_nonblocking_socket())
    , idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
    , listening_(false)
    , listen_addr_(listen_addr)
    , port_(port)
{
}

Acceptor::~Acceptor() {
    if (accept_fd_ >= 0) {
        ::close(accept_fd_);
    }
    if (idle_fd_ >= 0) {
        ::close(idle_fd_);
    }
}

void Acceptor::listen() {
    if (listening_) {
        return;
    }

    bind_and_listen();

    // 添加到事件循环并设置回调
    loop_->get_poller()->add_fd(accept_fd_, 
                               core::Poller::EVENT_READ, 
                               [this](int, uint32_t) { handle_read(); });
    listening_ = true;
}

void Acceptor::handle_read() {
    struct sockaddr_in peer_addr;
    socklen_t addr_len = sizeof(peer_addr);

    // 限制每次循环的接受数量以防止饥饿
    for (int i = 0; i < kMaxAcceptPerLoop; ++i) {
        int conn_fd = ::accept4(accept_fd_, 
                               reinterpret_cast<struct sockaddr*>(&peer_addr),
                               &addr_len,
                               SOCK_NONBLOCK | SOCK_CLOEXEC);
        
        if (conn_fd >= 0) {
            // 获得新连接
            std::string peer_ip = ::inet_ntoa(peer_addr.sin_addr);
            uint16_t peer_port = ntohs(peer_addr.sin_port);
            std::string peer_address = peer_ip + ":" + std::to_string(peer_port);
            
            if (new_connection_callback_) {
                new_connection_callback_(conn_fd, peer_address);
            } else {
                ::close(conn_fd);
            }
        } else {
            int saved_errno = errno;
            if (saved_errno != EAGAIN && saved_errno != EWOULDBLOCK) {
                if (saved_errno == EMFILE || saved_errno == ENFILE) {
                    // 打开文件太多 - EMFILE 保护
                    ::close(idle_fd_);  // 关闭保留的文件描述符
                    conn_fd = ::accept(accept_fd_, nullptr, nullptr);
                    if (conn_fd >= 0) {
                        ::close(conn_fd);  // 立即关闭连接
                    }
                    idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);  // 重新打开保留的文件描述符
                    std::cerr << "accept: " << strerror(saved_errno) << " - connection rejected" << std::endl;
                } else {
                    // 其他错误
                    std::cerr << "accept error: " << strerror(saved_errno) << std::endl;
                }
            }
            break;
        }
    }
}

int Acceptor::create_nonblocking_socket() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // 设置 SO_REUSEADDR
    int optval = 1;
    if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        ::close(sockfd);
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    // 设置 SO_REUSEPORT 以获得更好的负载均衡
    if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        // SO_REUSEPORT 在所有系统上都不可用，所以这不是致命的
        std::cerr << "Warning: SO_REUSEPORT not supported" << std::endl;
    }

    return sockfd;
}

void Acceptor::bind_and_listen() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(listen_addr_.c_str());

    if (listen_addr_ == "0.0.0.0" || listen_addr_.empty()) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (::inet_aton(listen_addr_.c_str(), &addr.sin_addr) == 0) {
            throw std::runtime_error("Invalid listen address: " + listen_addr_);
        }
    }

    if (::bind(accept_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind to " + listen_addr_ + ":" + std::to_string(port_));
    }

    if (::listen(accept_fd_, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

}  // namespace tzzero::net
