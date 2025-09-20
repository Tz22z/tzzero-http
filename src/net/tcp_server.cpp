#include "tzzero/net/tcp_server.h"
#include "tzzero/net/acceptor.h"
#include "tzzero/net/event_loop_thread_pool.h"
#include "tzzero/core/event_loop.h"
#include "tzzero/utils/logger.h"
#include <cassert>

namespace tzzero::net {

TcpServer::TcpServer(core::EventLoop* loop, const std::string& listen_addr,
                     uint16_t port, const std::string& name)
    : loop_(loop)
    , ip_port_(listen_addr + ":" + std::to_string(port))
    , name_(name)
    , acceptor_(std::make_unique<Acceptor>(loop, listen_addr, port))
    , thread_pool_(std::make_unique<EventLoopThreadPool>(loop))
    , started_(false)
    , next_conn_id_(1)
{
    acceptor_->set_new_connection_callback(
        [this](int sockfd, const std::string& peer_addr) {
            new_connection(sockfd, peer_addr);
        });
}

TcpServer::~TcpServer() {
    assert(loop_->is_in_loop_thread());
    LOG_DEBUG("TcpServer::~TcpServer [" << name_ << "] destructing");

    for (auto& item : connections_) {
        TcpConnectionPtr conn = item.second;
        conn->get_loop()->run_in_loop([conn]() {
            conn->connection_destroyed();
        });
    }
}

void TcpServer::start() {
    if (started_.exchange(true)) {
        return; // 已经启动
    }

    thread_pool_->start();

    assert(!acceptor_->listening());
    loop_->run_in_loop([this]() {
        acceptor_->listen();
    });

    LOG_INFO("TcpServer [" << name_ << "] started on " << ip_port_);
}

void TcpServer::stop() {
    // TODO: 实现优雅关闭
    LOG_INFO("TcpServer [" << name_ << "] stopping");
}

void TcpServer::set_thread_num(int num_threads) {
    assert(!started_);
    thread_pool_->set_thread_num(num_threads);
}

void TcpServer::new_connection(int sockfd, const std::string& peer_addr) {
    assert(loop_->is_in_loop_thread());

    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d", ip_port_.c_str(), next_conn_id_++);
    std::string conn_name = name_ + buf;

    LOG_INFO("TcpServer::new_connection [" << name_ << "] - new connection ["
             << conn_name << "] from " << peer_addr);

    // 选择一个EventLoop处理此连接
    core::EventLoop* io_loop = thread_pool_->get_next_loop();

    TcpConnectionPtr conn = std::make_shared<TcpConnection>(io_loop, conn_name, sockfd);
    connections_[conn_name] = conn;

    conn->set_message_callback(message_callback_);
    conn->set_close_callback([this](const TcpConnectionPtr& conn) {
        remove_connection(conn);
    });
    conn->set_write_complete_callback(write_complete_callback_);

    io_loop->run_in_loop([conn]() {
        conn->connection_established();
    });

    if (connection_callback_) {
        connection_callback_(conn);
    }
}

void TcpServer::remove_connection(const TcpConnectionPtr& conn) {
    loop_->run_in_loop([this, conn]() {
        remove_connection_in_loop(conn);
    });
}

void TcpServer::remove_connection_in_loop(const TcpConnectionPtr& conn) {
    assert(loop_->is_in_loop_thread());

    LOG_INFO("TcpServer::remove_connection_in_loop [" << name_ << "] - connection " << conn->get_name());

    size_t n = connections_.erase(conn->get_name());
    (void)n;
    assert(n == 1);

    core::EventLoop* io_loop = conn->get_loop();
    io_loop->queue_in_loop([conn]() {
        conn->connection_destroyed();
    });
}

} // namespace tzzero::net
