#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace tzzero::core {
class EventLoop;
}

namespace tzzero::net {

/**
 * EventLoop线程封装
 * 每个线程运行一个独立的EventLoop
 */
class EventLoopThread {
public:
    using ThreadInitCallback = std::function<void(core::EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback{});
    ~EventLoopThread();

    // 禁止拷贝
    EventLoopThread(const EventLoopThread&) = delete;
    EventLoopThread& operator=(const EventLoopThread&) = delete;

    /**
     * 启动线程并返回EventLoop指针
     */
    core::EventLoop* start_loop();

private:
    void thread_func();

    core::EventLoop* loop_;
    bool exiting_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};

/**
 * EventLoop线程池
 * 管理多个EventLoop工作线程，实现负载均衡
 */
class EventLoopThreadPool {
public:
    using ThreadInitCallback = std::function<void(core::EventLoop*)>;

    EventLoopThreadPool(core::EventLoop* base_loop);
    ~EventLoopThreadPool();

    // 禁止拷贝
    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool& operator=(const EventLoopThreadPool&) = delete;

    /**
     * 设置线程数（必须在start之前调用）
     */
    void set_thread_num(int num_threads) { num_threads_ = num_threads; }

    /**
     * 启动所有工作线程
     */
    void start(const ThreadInitCallback& cb = ThreadInitCallback{});

    /**
     * 获取下一个EventLoop（Round-Robin）
     */
    core::EventLoop* get_next_loop();

private:
    core::EventLoop* base_loop_;  // 主EventLoop
    bool started_;                 // 是否已启动
    int num_threads_;              // 线程数
    int next_;                     // 下一个线程索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_;  // 线程列表
    std::vector<core::EventLoop*> loops_;                    // EventLoop列表
};

} // namespace tzzero::net
