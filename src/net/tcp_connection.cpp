#include "tzzero/net/tcp_connection.h"
#include "tzzero/core/event_loop.h"
#include "tzzero/core/poller.h"
#include "tzzero/utils/logger.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cassert>

namespace tzzero::net {

TcpConnection::TcpConnection(core::EventLoop* loop, const std::string& name, int sockfd)
    : loop_(loop)
    , name_(name)
    , state_(CONNECTING)
    , socket_fd_(sockfd)
    , high_water_mark_(64 * 1024 * 1024)  // 64MB
{
    // 获取本地和对端地址
    struct sockaddr_in local_addr, peer_addr;
    socklen_t addr_len = sizeof(local_addr);
    
    if (::getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&local_addr), &addr_len) == 0) {
        char ip_buf[INET_ADDRSTRLEN];
        ::inet_ntop(AF_INET, &local_addr.sin_addr, ip_buf, INET_ADDRSTRLEN);
        local_addr_ = std::string(ip_buf) + ":" + std::to_string(ntohs(local_addr.sin_port));
    }

    addr_len = sizeof(peer_addr);
    if (::getpeername(sockfd, reinterpret_cast<struct sockaddr*>(&peer_addr), &addr_len) == 0) {
        char ip_buf[INET_ADDRSTRLEN];
        ::inet_ntop(AF_INET, &peer_addr.sin_addr, ip_buf, INET_ADDRSTRLEN);
        peer_addr_ = std::string(ip_buf) + ":" + std::to_string(ntohs(peer_addr.sin_port));
    }
    
    LOG_DEBUG("TcpConnection created: " << name_ << " fd=" << socket_fd_ 
              << " local=" << local_addr_ << " peer=" << peer_addr_);             
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG("TcpConnection destroyed: " << name_ << " fd=" << socket_fd_);
    assert(state_ == DISCONNECTED);
}

void TcpConnection::send(const void* data, size_t len) {
    if (state_ == CONNECTED) {
        if (loop_->is_in_loop_thread()) {
            send_in_loop(data, len);
        } else {
            std::string message(static_cast<const char*>(data), len);
            loop_->run_in_loop([this, message]() {
                send_in_loop(message.data(), message.size());
            });
        }
    }
}

void TcpConnection::send(const std::string& message) {
    send(message.data(), message.size());
}

void TcpConnection::send(utils::Buffer& buffer) {
    if (state_ == CONNECTED) {
        if (loop_->is_in_loop_thread()) {
            send_in_loop(buffer.peek(), buffer.readable_bytes());
            buffer.retrieve_all();
        } else {
            loop_->run_in_loop([this, &buffer]() {
                send_in_loop(buffer.peek(), buffer.readable_bytes());
                buffer.retrieve_all();
            });
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == CONNECTED) {
        state_ = DISCONNECTING;
        if (loop_->is_in_loop_thread()) {
            shutdown_in_loop();
        } else {
            loop_->run_in_loop([this]() {
                shutdown_in_loop();
            });
        }
    }
}

void TcpConnection::force_close() {
    if (state_ == CONNECTED || state_ == DISCONNECTING) {
        state_ = DISCONNECTING;
        if (loop_->is_in_loop_thread()) {
            force_close_in_loop();
        } else {
            loop_->run_in_loop([this]() {
                force_close_in_loop();
            });
        }
    }
}

void TcpConnection::set_high_water_mark_callback(const HighWaterMarkCallback& cb, size_t high_water_mark) {
    high_water_mark_callback_ = cb;
    high_water_mark_ = high_water_mark;
}

void TcpConnection::connection_established() {
    assert(loop_->is_in_loop_thread());
    assert(state_ == CONNECTING);
    
    state_ = CONNECTED;
    // 添加到事件循环用于读取
    loop_->get_poller()->add_fd(socket_fd_, core::Poller::EVENT_READ, 
        [this](int fd, uint32_t events) {
            if (events & core::Poller::EVENT_READ) {
                handle_read();
            }
            if (events & core::Poller::EVENT_WRITE) {
                handle_write();
            }
            if (events & core::Poller::EVENT_ERROR) {
                handle_error();
            }
        });
}

void TcpConnection::connection_destroyed() {
    assert(loop_->is_in_loop_thread());
    
    if (state_ == CONNECTED) {
        state_ = DISCONNECTED;
        loop_->get_poller()->remove_fd(socket_fd_);
        
        if (close_callback_) {
            close_callback_(shared_from_this());
        }
    }
}

void TcpConnection::set_tcp_no_delay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void TcpConnection::set_keep_alive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(socket_fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

void TcpConnection::handle_read() {
    assert(loop_->is_in_loop_thread());
    
    int saved_errno = 0;
    ssize_t n = input_buffer_.read_fd(socket_fd_, &saved_errno);
    
    if (n > 0) {
        if (message_callback_) {
            message_callback_(shared_from_this(), input_buffer_);
        }
    }
    else if (n == 0) {
        handle_close();
    } else {
        errno = saved_errno;
        LOG_ERROR("TcpConnection::handle_read error: " << strerror(saved_errno));
        handle_error();
    }
}

void TcpConnection::handle_write() {
    assert(loop_->is_in_loop_thread());
    
    if (state_ == CONNECTED) {
        int saved_errno = 0;
        ssize_t n = output_buffer_.write_fd(socket_fd_, &saved_errno);

        if (n > 0) {
            if (output_buffer_.readable_bytes() == 0) {
                loop_->get_poller()->modify_fd(socket_fd_, core::Poller::EVENT_READ, 
                    [this](int fd, uint32_t events) {
                        if (events & core::Poller::EVENT_READ) {
                            handle_read();
                        }
                        if (events & core::Poller::EVENT_WRITE) {
                            handle_write();
                        }
                        if (events & core::Poller::EVENT_ERROR) {
                            handle_error();
                        }
                    });

                if (write_complete_callback_) {
                    loop_->queue_in_loop([this]() {
                        write_complete_callback_(shared_from_this());
                    });
                }

                if (state_ == DISCONNECTING) {
                    shutdown_in_loop();
                }
            }
        } else {
            LOG_ERROR("TcpConnection::handle_write error: " << strerror(saved_errno));
        }
    }
}


void TcpConnection::handle_close() {
    assert(loop_->is_in_loop_thread());
    assert(state_ == CONNECTED || state_ == DISCONNECTING);
    
    state_ = DISCONNECTED;
    loop_->get_poller()->remove_fd(socket_fd_);
    
    auto guard_this = shared_from_this();
    if (close_callback_) {
        close_callback_(guard_this);
    }
}

void TcpConnection::handle_error() {
    assert(loop_->is_in_loop_thread());
    
    int err = 0;
    socklen_t len = sizeof(err);
    if (err != 0) {
        LOG_ERROR("TcpConnection::handle_error - SO_ERROR: " << strerror(err));
    }

    handle_close();
}

void TcpConnection::send_in_loop(const void* data, size_t len) {
    assert(loop_->is_in_loop_thread());
    
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool fault_error = false;
    
    if (state_ == CONNECTED && output_buffer_.readable_bytes() == 0) {
        // 尝试直接写入
        nwrote = ::write(socket_fd_, data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && write_complete_callback_) {
                loop_->queue_in_loop([this]() {
                    write_complete_callback_(shared_from_this());
                });
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::send_in_loop write error: " << strerror(errno));
                if (errno == EPIPE || errno == ECONNRESET) {
                    fault_error = true;
                }
            }
        }
    }
    
    assert(remaining <= len);
    if (!fault_error && remaining > 0) {
        size_t old_len = output_buffer_.readable_bytes();
        if (old_len + remaining >= high_water_mark_ && old_len < high_water_mark_ && high_water_mark_callback_) {
            loop_->queue_in_loop([this, old_len, remaining]() {
                high_water_mark_callback_(shared_from_this(), old_len + remaining);
            });
        }
        
        output_buffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        
                        // 启用写入
                loop_->get_poller()->modify_fd(socket_fd_, 
                                             core::Poller::EVENT_READ | core::Poller::EVENT_WRITE, 
                                             [this](int fd, uint32_t events) {
                                                 if (events & core::Poller::EVENT_READ) {
                                                     handle_read();
                                                 }
                                                 if (events & core::Poller::EVENT_WRITE) {
                                                     handle_write();
                                                 }
                                                 if (events & core::Poller::EVENT_ERROR) {
                                                     handle_error();
                                                 }
                                             });
    }
}

void TcpConnection::shutdown_in_loop() {
    assert(loop_->is_in_loop_thread());
    
    if (output_buffer_.readable_bytes() == 0) {
        ::shutdown(socket_fd_, SHUT_WR);
    }
}

void TcpConnection::force_close_in_loop() {
    assert(loop_->is_in_loop_thread());
    
    if (state_ == CONNECTED || state_ == DISCONNECTING) {
        handle_close();
    }
}

}  // namespace tzzero::net
