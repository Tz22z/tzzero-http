#include "tzzero/net/event_loop_thread_pool.h"
#include "tzzero/core/event_loop.h"
#include <cassert>

namespace tzzero::net {

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
    : loop_(nullptr)
    , exiting_(false)
    , callback_(cb)
{
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        if (thread_.joinable()) {
            thread_.join();
        }
    }
}

core::EventLoop* EventLoopThread::start_loop() {
    assert(!thread_.joinable());
    thread_ = std::thread(&EventLoopThread::thread_func, this);

    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait(lock);
        }
    }

    return loop_;
}

void EventLoopThread::thread_func() {
    core::EventLoop loop;

    if (callback_) {
        callback_(&loop);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();

    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

EventLoopThreadPool::EventLoopThreadPool(core::EventLoop* base_loop)
    : base_loop_(base_loop)
    , started_(false)
    , num_threads_(0)
    , next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool() {
    // EventLoopThread析构函数会处理清理
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    assert(!started_);
    assert(base_loop_->is_in_loop_thread());

    started_ = true;

    for (int i = 0; i < num_threads_; ++i) {
        auto thread = std::make_unique<EventLoopThread>(cb);
        loops_.push_back(thread->start_loop());
        threads_.push_back(std::move(thread));
    }

    if (num_threads_ == 0 && cb) {
        cb(base_loop_);
    }
}

core::EventLoop* EventLoopThreadPool::get_next_loop() {
    assert(started_);
    base_loop_->is_in_loop_thread();

    core::EventLoop* loop = base_loop_;

    if (!loops_.empty()) {
        // Round-robin负载均衡
        loop = loops_[next_];
        ++next_;
        if (static_cast<size_t>(next_) >= loops_.size()) {
            next_ = 0;
        }
    }

    return loop;
}

} // namespace tzzero::net
