#include "tzzero/utils/thread_pool.h"

namespace tzzero::utils {

ThreadPool::ThreadPool(size_t num_threads) : stop_flag_(false) {
    for (size_t i = 0; i < num_threads; ++i) {
        threads_.emplace_back([this] {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    condition_.wait(lock, [this] { return stop_flag_ || !tasks_.empty(); });

                    if (stop_flag_ && tasks_.empty()) {
                        return;
                    }

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_flag_ = true;
    }

    condition_.notify_all();

    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

} // namespace tzzero::utils
