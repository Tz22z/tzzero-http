#include "tzzero/core/timer_queue.h"
#include "tzzero/core/event_loop.h"
#include "tzzero/core/poller.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

namespace tzzero::core {

namespace {
int create_timerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        throw std::runtime_error("Failed to create timer fd");
    }
    return timerfd;
}

double now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

void read_timerfd(int timerfd) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    // TODO: 应该检查读取是否成功，但暂时忽略
    (void)n;
}

void reset_timerfd(int timerfd, double expiration) {
    struct itimerspec new_value;
    memset(&new_value, 0, sizeof(new_value));
    
    int64_t microseconds = static_cast<int64_t>(expiration * 1000000.0);
    new_value.it_value.tv_sec = static_cast<time_t>(microseconds / 1000000);
    new_value.it_value.tv_nsec = static_cast<long>((microseconds % 1000000) * 1000);
    
    int ret = ::timerfd_settime(timerfd, 0, &new_value, nullptr);
    if (ret) {
        // TODO: 应该处理错误，但先让它能跑
    }
}

} // anonymous namespace

std::atomic<uint64_t> Timer::s_num_created_{0};

Timer::Timer(double when, double interval, TimerCallback cb)
    : callback_(std::move(cb))
    , expiration_(when)
    , interval_(interval)
    , repeat_(interval > 0.0)
    , sequence_(++s_num_created_)
{
}

void Timer::restart(double now) {
    if (repeat_) {
        expiration_ = now + interval_;
    } else {
        expiration_ = 0.0;
    }
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop)
    , timer_fd_(create_timerfd())
    , calling_expired_timers_(false)
{
    // Add timer fd to poller
    loop_->get_poller()->add_fd(timer_fd_, Poller::EVENT_READ, [this](int, uint32_t) {
        handle_timer_fd();
    });
}

TimerQueue::~TimerQueue() {
    loop_->get_poller()->remove_fd(timer_fd_);
    ::close(timer_fd_);
    
    // 简单清理 - 可能有内存泄漏，后面再修复
    for (const auto& timer_pair : timers_) {
        delete timer_pair.second;
    }
}

uint64_t TimerQueue::add_timer(double when, double interval, TimerCallback cb) {
    auto timer = std::make_unique<Timer>(now() + when, interval, std::move(cb));
    uint64_t seq = timer->sequence();
    
    // 简化版本 - 直接在当前线程添加，不考虑线程安全
    add_timer_in_loop(std::move(timer));
    
    return seq;
}

void TimerQueue::cancel_timer(uint64_t timer_id) {
    // 简化实现 - 后面再优化
    cancel_in_loop(timer_id);
}

int TimerQueue::get_next_timeout() const {
    if (timers_.empty()) {
        return -1;
    }
    
    double next_expire = timers_.begin()->first;
    double now_time = now();
    
    if (next_expire <= now_time) {
        return 0;
    }
    
    return static_cast<int>((next_expire - now_time) * 1000);
}

void TimerQueue::process_expired_timers() {
    calling_expired_timers_ = true;
    
    double now_time = now();
    std::vector<Entry> expired = get_expired(now_time);
    
    // 简单执行，不处理异常情况
    for (const auto& entry : expired) {
        entry.second->run();
    }
    
    reset(expired, now_time);
    calling_expired_timers_ = false;
}

void TimerQueue::add_timer_in_loop(TimerPtr timer) {
    bool earliest_changed = insert(timer.get());
    
    if (earliest_changed && !timers_.empty()) {
        reset_timerfd(timer_fd_, timer->when());
    }
    
    timer.release(); // 转移所有权
}

void TimerQueue::cancel_in_loop(uint64_t timer_id) {
    // 简化版本 - 线性搜索，效率不高
    auto it = active_timers_.begin();
    while (it != active_timers_.end()) {
        if (it->second == timer_id) {
            Timer* timer = it->first;
            active_timers_.erase(it);
            timers_.erase({timer->when(), timer});
            delete timer;
            break;
        }
        ++it;
    }
}

void TimerQueue::handle_timer_fd() {
    read_timerfd(timer_fd_);
    process_expired_timers();
}

std::vector<TimerQueue::Entry> TimerQueue::get_expired(double now_time) {
    std::vector<Entry> expired;
    Entry sentry(now_time, reinterpret_cast<Timer*>(UINTPTR_MAX));
    auto end = timers_.lower_bound(sentry);
    
    std::copy(timers_.begin(), end, std::back_inserter(expired));
    timers_.erase(timers_.begin(), end);
    
    // 从active_timers_中移除
    for (const auto& entry : expired) {
        ActiveTimer timer(entry.second, entry.second->sequence());
        active_timers_.erase(timer);
    }
    
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, double now_time) {
    for (const auto& entry : expired) {
        if (entry.second->repeat()) {
            entry.second->restart(now_time);
            insert(entry.second);
        } else {
            delete entry.second;
        }
    }
    
    if (!timers_.empty()) {
        double next_expire = timers_.begin()->second->when();
        reset_timerfd(timer_fd_, next_expire);
    }
}

bool TimerQueue::insert(Timer* timer) {
    bool earliest_changed = false;
    double when = timer->when();
    auto it = timers_.begin();
    
    if (it == timers_.end() || when < it->first) {
        earliest_changed = true;
    }
    
    timers_.insert({when, timer});
    active_timers_.insert({timer, timer->sequence()});
    
    return earliest_changed;
}

} // namespace tzzero::core

