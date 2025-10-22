#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <cstddef>

namespace tzzero::utils {

template<typename T>
class MemoryPool {
public:
    explicit MemoryPool(size_t block_size = 1024);
    ~MemoryPool();

    // Non-copyable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Allocate object
    T* allocate();

    // Deallocate object
    void deallocate(T* ptr);

    // Get statistics
    size_t allocated_count() const { return allocated_count_; }
    size_t total_capacity() const { return blocks_.size() * block_size_; }

private:
    struct Block {
        std::unique_ptr<T[]> data;
        std::vector<bool> used;
        size_t free_count;

        explicit Block(size_t size)
            : data(std::make_unique<T[]>(size))
            , used(size, false)
            , free_count(size) {}
    };

    void add_block();

    const size_t block_size_;
    std::vector<std::unique_ptr<Block>> blocks_;
    size_t allocated_count_;
    mutable std::mutex mutex_;
};

template<typename T>
MemoryPool<T>::MemoryPool(size_t block_size)
    : block_size_(block_size)
    , allocated_count_(0)
{
    add_block();
}

template<typename T>
MemoryPool<T>::~MemoryPool() = default;

template<typename T>
T* MemoryPool<T>::allocate() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& block : blocks_) {
        if (block->free_count > 0) {
            for (size_t i = 0; i < block_size_; ++i) {
                if (!block->used[i]) {
                    block->used[i] = true;
                    --block->free_count;
                    ++allocated_count_;
                    return &block->data[i];
                }
            }
        }
    }

    // No free slots, add new block
    add_block();
    auto& new_block = blocks_.back();
    new_block->used[0] = true;
    --new_block->free_count;
    ++allocated_count_;
    return &new_block->data[0];
}

template<typename T>
void MemoryPool<T>::deallocate(T* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& block : blocks_) {
        T* block_start = block->data.get();
        T* block_end = block_start + block_size_;

        if (ptr >= block_start && ptr < block_end) {
            size_t index = ptr - block_start;
            if (block->used[index]) {
                block->used[index] = false;
                ++block->free_count;
                --allocated_count_;
            }
            return;
        }
    }
}

template<typename T>
void MemoryPool<T>::add_block() {
    blocks_.push_back(std::make_unique<Block>(block_size_));
}

} // namespace tzzero::utils
