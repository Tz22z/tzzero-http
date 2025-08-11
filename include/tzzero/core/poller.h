#pragma once

#include <vector>
#include <memory>
#include <functional>

namespace tzzero::core {

class EventLoop;

struct PollEvent {
    int fd;
    uint32_t events;
    std::function<void(int, uint32_t)> callback;
};

class Poller {
public:
    using EventCallback = std::function<void(int fd, uint32_t events)>;
    
    explicit Poller(EventLoop* loop);
    virtual ~Poller() = default;

    // Non-copyable
    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    // Poll for events, timeout in milliseconds
    virtual int poll(int timeout_ms, std::vector<PollEvent>& active_events) = 0;
    
    // Add/modify/remove file descriptor
    virtual void add_fd(int fd, uint32_t events, std::function<void(int, uint32_t)> callback) = 0;
    virtual void modify_fd(int fd, uint32_t events, std::function<void(int, uint32_t)> callback) = 0;
    virtual void remove_fd(int fd) = 0;
    
    // Event flags
    static constexpr uint32_t EVENT_READ = 0x001;
    static constexpr uint32_t EVENT_WRITE = 0x004;
    static constexpr uint32_t EVENT_ERROR = 0x008;
    static constexpr uint32_t EVENT_HUP = 0x010;
    static constexpr uint32_t EVENT_EDGE_TRIGGERED = 0x80000000;

protected:
    EventLoop* owner_loop_;
};

// Factory function to create appropriate poller
std::unique_ptr<Poller> create_poller(EventLoop* loop);

} // namespace tzzero::core



