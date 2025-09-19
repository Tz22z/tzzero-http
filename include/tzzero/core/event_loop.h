#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace tzzero::core {

class Poller;
class TimerQueue;

using EventCallback = std::function<void()>;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    // 不可拷贝
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    // 在当前线程启动事件循环
    void loop();
    
    // 停止事件循环
    void quit();
    
    // 检查是否在循环线程中运行
    bool is_in_loop_thread() const;
    
    // 在循环线程中执行回调
    void run_in_loop(EventCallback cb);
    void queue_in_loop(EventCallback cb);
    
    // 定时器操作
    uint64_t run_after(double delay_seconds, EventCallback cb);
    uint64_t run_every(double interval_seconds, EventCallback cb);
    void cancel_timer(uint64_t timer_id);
    
    // 获取轮询器用于连接管理
    Poller* get_poller() const { return poller_.get(); }
    
    // 线程管理
    std::thread::id get_thread_id() const { return thread_id_; }

private:
    void wake_up();
    void handle_wake_up();
    void do_pending_functors();

    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timer_queue_;
    
    std::atomic<bool> looping_{false};
    std::atomic<bool> quit_{false};
    std::thread::id thread_id_;
    
    // 用于跨线程调用
    int wakeup_fd_;
    std::mutex mutex_;
    std::vector<EventCallback> pending_functors_;
    bool calling_pending_functors_{false};
};

}  // namespace tzzero::core







