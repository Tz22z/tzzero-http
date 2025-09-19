#include "tzzero/core/epoll_poller.h"
#include "tzzero/core/event_loop.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cassert>
#include <iostream>

namespace tzzero::core {

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop)
    , epoll_fd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if (epoll_fd_ < 0) {
        throw std::runtime_error("Failed to create epoll fd");
    }
}

EpollPoller::~EpollPoller() {
    ::close(epoll_fd_);
}

int EpollPoller::poll(int timeout_ms, std::vector<PollEvent>& active_events) {
    int num_events = ::epoll_wait(epoll_fd_, events_.data(), 
                                  static_cast<int>(events_.size()), timeout_ms);
    
    if (num_events > 0) {
        active_events.reserve(num_events);
        
        for (int i = 0; i < num_events; ++i) {
            int fd = events_[i].data.fd;
            uint32_t revents = epoll_to_events(events_[i].events);
            
            auto it = fd_callbacks_.find(fd);
            if (it != fd_callbacks_.end()) {
                PollEvent event;
                event.fd = fd;
                event.events = revents;
                event.callback = it->second;
                active_events.push_back(event);
            }
        }

        if (static_cast<size_t>(num_events) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    }

    return num_events;
}

void EpollPoller::add_fd(int fd, uint32_t events, std::function<void(int, uint32_t)> callback) {
    epoll_event event;
    event.events = events_to_epoll(events);
    event.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        throw std::runtime_error("epoll_ctl ADD failed");
    }
    
    fd_callbacks_[fd] = std::move(callback);
}

void EpollPoller::modify_fd(int fd, uint32_t events, std::function<void(int, uint32_t)> callback) {
    epoll_event event;
    event.events = events_to_epoll(events);
    event.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) < 0) {
        throw std::runtime_error("epoll_ctl MOD failed");
    }
    
    fd_callbacks_[fd] = std::move(callback);
}

void EpollPoller::remove_fd(int fd) {
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        throw std::runtime_error("epoll_ctl DEL failed");
    }
    
    fd_callbacks_.erase(fd);
}

uint32_t EpollPoller::events_to_epoll(uint32_t events) const {
    uint32_t epoll_events = 0;
    
    if (events & EVENT_READ) {
        epoll_events |= EPOLLIN | EPOLLPRI;
    }
    if (events & EVENT_WRITE) {
        epoll_events |= EPOLLOUT;
    }
    if (events & EVENT_EDGE_TRIGGERED) {
        epoll_events |= EPOLLET;
    }
    
    return epoll_events;
}

uint32_t EpollPoller::epoll_to_events(uint32_t epoll_events) const {
    uint32_t events = 0;
    
    if (epoll_events & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        events |= EVENT_READ;
    }
    if (epoll_events & EPOLLOUT) {
        events |= EVENT_WRITE;
    }
    if (epoll_events & (EPOLLERR | EPOLLHUP)) {
        events |= EVENT_ERROR;
    }
    
    return events;
}

}  // namespace tzzero::core
