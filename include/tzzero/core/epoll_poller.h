#pragma once

#include "poller.h"
#include <sys/epoll.h>
#include <vector>
#include <unordered_map>

namespace tzzero::core {

class EpollPoller : public Poller {
public:
    explicit EpollPoller(EventLoop* loop);
    ~EpollPoller() override;

    int poll(int timeout_ms, std::vector<PollEvent>& active_events) override;
    void add_fd(int fd, uint32_t events, std::function<void(int, uint32_t)> callback) override;
    void modify_fd(int fd, uint32_t events, std::function<void(int, uint32_t)> callback) override;
    void remove_fd(int fd) override;

private:
    uint32_t events_to_epoll(uint32_t events) const;
    uint32_t epoll_to_events(uint32_t epoll_events) const;
    
    int epoll_fd_;
    std::vector<epoll_event> events_;
    std::unordered_map<int, std::function<void(int, uint32_t)>> fd_callbacks_;
    static constexpr int kInitEventListSize = 16;
};

}  // namespace tzzero::core
