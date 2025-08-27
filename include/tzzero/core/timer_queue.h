#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <set>
#include <atomic>

namespace tzzero::core {

class EventLoop;

using TimerCallback = std::function<void()>;

class Timer {
public:
    Timer(double when, double interval, TimerCallback cb);
    
    void run() const { callback_(); }
    bool repeat() const { return repeat_; }
    double when() const { return expiration_; }
    uint64_t sequence() const { return sequence_; }
    
    void restart(double now);
    
    static uint64_t num_created() { return s_num_created_.load(); }

private:
    const TimerCallback callback_;
    double expiration_;
    const double interval_;
    const bool repeat_;
    const uint64_t sequence_;
    
    static std::atomic<uint64_t> s_num_created_;
};

using TimerPtr = std::unique_ptr<Timer>;

class TimerQueue {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    // 不可拷贝
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;

    // 添加定时器，返回定时器ID
    uint64_t add_timer(double when, double interval, TimerCallback cb);
    
    // 取消定时器
    void cancel_timer(uint64_t timer_id);
    
    // 获取下一次超时时间（毫秒），-1表示无超时
    int get_next_timeout() const;
    
    // 处理已过期的定时器
    void process_expired_timers();

private:
    using Entry = std::pair<double, Timer*>;
    using TimerList = std::set<Entry>;
    using ActiveTimer = std::pair<Timer*, uint64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    void add_timer_in_loop(TimerPtr timer);
    void cancel_in_loop(uint64_t timer_id);
    void handle_timer_fd();
    
    std::vector<Entry> get_expired(double now);
    void reset(const std::vector<Entry>& expired, double now);
    
    bool insert(Timer* timer);

    EventLoop* loop_;
    const int timer_fd_;
    TimerList timers_;
    ActiveTimerSet active_timers_;
    std::atomic<bool> calling_expired_timers_;
    ActiveTimerSet canceling_timers_;
};

}  // namespace tzzero::core
