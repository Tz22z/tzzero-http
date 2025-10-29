#include <gtest/gtest.h>
#include "tzzero/utils/memory_pool.h"
#include <thread>
#include <vector>

using namespace tzzero::utils;

struct TestObject {
    int value;
    double data;
    TestObject() : value(0), data(0.0) {}
    TestObject(int v, double d) : value(v), data(d) {}
};

class MemoryPoolTest : public ::testing::Test {
protected:
    static constexpr size_t kBlockSize = 10;
};

TEST_F(MemoryPoolTest, AllocateAndDeallocate) {
    MemoryPool<int> pool(kBlockSize);

    int* ptr = pool.allocate();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(pool.allocated_count(), 1);

    *ptr = 42;
    EXPECT_EQ(*ptr, 42);

    pool.deallocate(ptr);
    EXPECT_EQ(pool.allocated_count(), 0);
}

TEST_F(MemoryPoolTest, AllocateMultiple) {
    MemoryPool<int> pool(kBlockSize);
    std::vector<int*> ptrs;

    for (int i = 0; i < 5; ++i) {
        int* ptr = pool.allocate();
        ASSERT_NE(ptr, nullptr);
        *ptr = i;
        ptrs.push_back(ptr);
    }

    EXPECT_EQ(pool.allocated_count(), 5);

    for (size_t i = 0; i < ptrs.size(); ++i) {
        EXPECT_EQ(*ptrs[i], static_cast<int>(i));
    }

    for (auto ptr : ptrs) {
        pool.deallocate(ptr);
    }

    EXPECT_EQ(pool.allocated_count(), 0);
}

TEST_F(MemoryPoolTest, AllocateBeyondBlockSize) {
    MemoryPool<int> pool(kBlockSize);
    std::vector<int*> ptrs;

    // 分配超过一个块的容量
    for (int i = 0; i < kBlockSize * 2 + 5; ++i) {
        int* ptr = pool.allocate();
        ASSERT_NE(ptr, nullptr);
        *ptr = i;
        ptrs.push_back(ptr);
    }

    EXPECT_EQ(pool.allocated_count(), kBlockSize * 2 + 5);
    EXPECT_GE(pool.total_capacity(), kBlockSize * 2);

    for (auto ptr : ptrs) {
        pool.deallocate(ptr);
    }

    EXPECT_EQ(pool.allocated_count(), 0);
}

TEST_F(MemoryPoolTest, ReuseFreedMemory) {
    MemoryPool<int> pool(kBlockSize);

    int* ptr1 = pool.allocate();
    *ptr1 = 100;
    pool.deallocate(ptr1);

    int* ptr2 = pool.allocate();
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(pool.allocated_count(), 1);

    pool.deallocate(ptr2);
}

TEST_F(MemoryPoolTest, ComplexObject) {
    MemoryPool<TestObject> pool(kBlockSize);

    TestObject* obj = pool.allocate();
    ASSERT_NE(obj, nullptr);

    obj->value = 42;
    obj->data = 3.14;

    EXPECT_EQ(obj->value, 42);
    EXPECT_DOUBLE_EQ(obj->data, 3.14);

    pool.deallocate(obj);
    EXPECT_EQ(pool.allocated_count(), 0);
}

TEST_F(MemoryPoolTest, ThreadSafety) {
    MemoryPool<int> pool(100);
    constexpr int kNumThreads = 4;
    constexpr int kAllocationsPerThread = 25;

    auto worker = [&pool]() {
        std::vector<int*> local_ptrs;
        for (int i = 0; i < kAllocationsPerThread; ++i) {
            int* ptr = pool.allocate();
            ASSERT_NE(ptr, nullptr);
            *ptr = i;
            local_ptrs.push_back(ptr);
        }
        for (auto ptr : local_ptrs) {
            pool.deallocate(ptr);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(pool.allocated_count(), 0);
}

TEST_F(MemoryPoolTest, DeallocateNullptr) {
    MemoryPool<int> pool(kBlockSize);

    EXPECT_NO_THROW(pool.deallocate(nullptr));
    EXPECT_EQ(pool.allocated_count(), 0);
}

TEST_F(MemoryPoolTest, Statistics) {
    MemoryPool<int> pool(kBlockSize);

    EXPECT_EQ(pool.allocated_count(), 0);
    EXPECT_EQ(pool.total_capacity(), kBlockSize);

    std::vector<int*> ptrs;
    for (size_t i = 0; i < kBlockSize; ++i) {
        ptrs.push_back(pool.allocate());
    }

    EXPECT_EQ(pool.allocated_count(), kBlockSize);

    // 分配一个会触发新块创建
    ptrs.push_back(pool.allocate());
    EXPECT_EQ(pool.allocated_count(), kBlockSize + 1);
    EXPECT_GE(pool.total_capacity(), kBlockSize * 2);

    for (auto ptr : ptrs) {
        pool.deallocate(ptr);
    }
}
