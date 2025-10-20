#include <gtest/gtest.h>
#include "tzzero/utils/thread_pool.h"
#include <chrono>
#include <atomic>

using namespace tzzero::utils;

class ThreadPoolTest : public ::testing::Test {
protected:
    static constexpr size_t kNumThreads = 4;
};

TEST_F(ThreadPoolTest, CreateAndDestroy) {
    EXPECT_NO_THROW({
        ThreadPool pool(kNumThreads);
        EXPECT_EQ(pool.size(), kNumThreads);
    });
}

TEST_F(ThreadPoolTest, SubmitSimpleTask) {
    ThreadPool pool(kNumThreads);

    auto future = pool.submit([]() {
        return 42;
    });

    EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, SubmitTaskWithArguments) {
    ThreadPool pool(kNumThreads);

    auto future = pool.submit([](int a, int b) {
        return a + b;
    }, 10, 20);

    EXPECT_EQ(future.get(), 30);
}

TEST_F(ThreadPoolTest, SubmitMultipleTasks) {
    ThreadPool pool(kNumThreads);
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([i]() {
            return i * i;
        }));
    }

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
    }
}

TEST_F(ThreadPoolTest, ConcurrentExecution) {
    ThreadPool pool(kNumThreads);
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([&counter]() {
            counter++;
        }));
    }

    for (auto& future : futures) {
        future.get();
    }

    EXPECT_EQ(counter.load(), 100);
}

TEST_F(ThreadPoolTest, TaskWithException) {
    ThreadPool pool(kNumThreads);

    auto future = pool.submit([]() -> int {
        throw std::runtime_error("Test exception");
    });

    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST_F(ThreadPoolTest, StopPool) {
    ThreadPool pool(kNumThreads);

    pool.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    EXPECT_NO_THROW(pool.stop());
}

TEST_F(ThreadPoolTest, CannotSubmitAfterStop) {
    ThreadPool pool(kNumThreads);
    pool.stop();

    EXPECT_THROW(
        pool.submit([]() { return 42; }),
        std::runtime_error
    );
}

TEST_F(ThreadPoolTest, TaskOrdering) {
    ThreadPool pool(1);  // 单线程确保顺序执行
    std::vector<int> results;
    std::mutex results_mutex;
    std::vector<std::future<void>> futures;

    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([i, &results, &results_mutex]() {
            std::lock_guard<std::mutex> lock(results_mutex);
            results.push_back(i);
        }));
    }

    for (auto& future : futures) {
        future.get();
    }

    EXPECT_EQ(results.size(), 10);
    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_EQ(results[i], i);
    }
}

TEST_F(ThreadPoolTest, LongRunningTasks) {
    ThreadPool pool(kNumThreads);
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return i;
        }));
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futures[i].get(), i);
    }
}
