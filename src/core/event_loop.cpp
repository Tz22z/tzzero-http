#include "tzzero/core/event_loop.h"
#include "tzzero/core/poller.h"
#include "tzzero/core/timer_queue.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <cassert>
#include <iostream>

namespace tzzero::core {

namespace {
thread_local EventLoop* t_loop_in_this_thread = nullptr;
}

EventLoop::EventLoop()
    : poller_(create_poller(this))
    , timer_queue_(std::make_unique<TimerQueue>(this))
    , thread_id_(std::this_thread::get_id())
    , wakeup_fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))
{
    if (wakeup_fd_ < 0) {
        throw std::runtime_error("Failed to create eventfd");
    }

    if (t_loop_in_this_thread) {
        throw std::runtime_error("Another EventLoop exists in this thread");
    } else {
        t_loop_in_this_thread = this;
    }

    // 将唤醒文件描述符添加到轮询器
    poller_->add_fd(wakeup_fd_, Poller::EVENT_READ, [this](int, uint32_t) {
        handle_wake_up();
    });
}

EventLoop::~EventLoop() {
    ::close(wakeup_fd_);
    t_loop_in_this_thread = nullptr;
}

void EventLoop::loop() {
    assert(!looping_);
    assert(is_in_loop_thread());
    
    looping_ = true;
    quit_ = false;

    std::vector<PollEvent> active_events;
    
    while (!quit_) {
        active_events.clear();
        
        // 带超时轮询事件
        int timeout_ms = timer_queue_->get_next_timeout();
        int num_events = poller_->poll(timeout_ms, active_events);
        
        if (num_events < 0) {
            // 简单处理错误 - 真实项目中这里会记录日志
            std::cerr << "Poller error" << std::endl;
            break;
        }

        // 处理定时器事件
        timer_queue_->process_expired_timers();

        // 处理 I/O 事件
        for (const auto& event : active_events) {
            if (event.callback) {
                event.callback(event.fd, event.events);
            }
        }

        // 处理待执行的函数对象
        do_pending_functors();
    }

    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    if (!is_in_loop_thread()) {
        wake_up();
    }
}

bool EventLoop::is_in_loop_thread() const {
    return thread_id_ == std::this_thread::get_id();
}

void EventLoop::run_in_loop(EventCallback cb) {
    if (is_in_loop_thread()) {
        cb();
    } else {
        queue_in_loop(std::move(cb));
    }
}

void EventLoop::queue_in_loop(EventCallback cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_functors_.push_back(std::move(cb));
    }

    // 简化版本 - 总是唤醒，效率不是最优但更安全
    wake_up();
}

uint64_t EventLoop::run_after(double delay_seconds, EventCallback cb) {
    return timer_queue_->add_timer(delay_seconds, 0.0, std::move(cb));
}

uint64_t EventLoop::run_every(double interval_seconds, EventCallback cb) {
    return timer_queue_->add_timer(interval_seconds, interval_seconds, std::move(cb));
}

void EventLoop::cancel_timer(uint64_t timer_id) {
    timer_queue_->cancel_timer(timer_id);
}

void EventLoop::wake_up() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeup_fd_, &one, sizeof(one));
    // 暂时不处理写入失败的情况
    (void)n;
}

void EventLoop::handle_wake_up() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeup_fd_, &one, sizeof(one));
    // 暂时不处理读取失败的情况
    (void)n;
}

void EventLoop::do_pending_functors() {
    std::vector<EventCallback> functors;
    calling_pending_functors_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pending_functors_);
    }

    // 简单执行所有回调，不处理异常
    for (const auto& functor : functors) {
        functor();
    }
    
    calling_pending_functors_ = false;
}

}  // namespace tzzero::core
