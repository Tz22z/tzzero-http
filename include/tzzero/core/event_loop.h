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

    // Non-copyable
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    // Start the event loop in current thread
    void loop();
    
    // Stop the event loop
    void quit();
    
    // Check if running in loop thread
    bool is_in_loop_thread() const;
    
    // Execute callback in loop thread
    void run_in_loop(EventCallback cb);
    void queue_in_loop(EventCallback cb);
    
    // Timer operations
    uint64_t run_after(double delay_seconds, EventCallback cb);
    uint64_t run_every(double interval_seconds, EventCallback cb);
    void cancel_timer(uint64_t timer_id);
    
    // Get poller for connection management
    Poller* get_poller() const { return poller_.get(); }
    
    // Thread management
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
    
    // For cross-thread calls
    int wakeup_fd_;
    std::mutex mutex_;
    std::vector<EventCallback> pending_functors_;
    bool calling_pending_functors_{false};
};

} // namespace tzzero::core
