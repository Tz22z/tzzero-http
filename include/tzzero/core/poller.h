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

    // 不可拷贝
    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    // 轮询事件，超时时间以毫秒为单位
    virtual int poll(int timeout_ms, std::vector<PollEvent>& active_events) = 0;
    
    // 添加/修改/删除文件描述符
    virtual void add_fd(int fd, uint32_t events, std::function<void(int, uint32_t)> callback) = 0;
    virtual void modify_fd(int fd, uint32_t events, std::function<void(int, uint32_t)> callback) = 0;
    virtual void remove_fd(int fd) = 0;
    
    // 事件标志
    static constexpr uint32_t EVENT_READ = 0x001;
    static constexpr uint32_t EVENT_WRITE = 0x004;
    static constexpr uint32_t EVENT_ERROR = 0x008;
    static constexpr uint32_t EVENT_HUP = 0x010;
    static constexpr uint32_t EVENT_EDGE_TRIGGERED = 0x80000000;

protected:
    EventLoop* owner_loop_;
};

// 工厂函数，用于创建适当的轮询器
std::unique_ptr<Poller> create_poller(EventLoop* loop);

}  // namespace tzzero::core







