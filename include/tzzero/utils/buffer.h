#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <cstddef>
#include <algorithm>
#include <sys/uio.h>

namespace tzzero::utils {

// 高性能缓冲区，支持零拷贝优化
class Buffer {
public:
    static constexpr size_t kCheapPrepend = 8;
    static constexpr size_t kInitialSize = 1024;

    explicit Buffer(size_t initial_size = kInitialSize);
    ~Buffer() = default;

    // 可拷贝和可移动
    Buffer(const Buffer& other);
    Buffer& operator=(const Buffer& other);
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    // 大小和容量
    size_t readable_bytes() const { return write_index_ - read_index_; }
    size_t writable_bytes() const { return buffer_.size() - write_index_; }
    size_t prependable_bytes() const { return read_index_; }
    size_t capacity() const { return buffer_.size(); }

    // 数据访问
    const char* peek() const { return begin() + read_index_; }
    char* begin_write() { return begin() + write_index_; }
    const char* begin_write() const { return begin() + write_index_; }

    // 读取操作
    void retrieve(size_t len);
    void retrieve_all() { read_index_ = write_index_ = kCheapPrepend; }
    std::string retrieve_as_string(size_t len);
    std::string retrieve_all_as_string();

    // 写入操作
    void append(const char* data, size_t len);
    void append(const std::string& str) { append(str.data(), str.size()); }
    void append(std::string_view str) { append(str.data(), str.size()); }
    void append(const void* data, size_t len) { append(static_cast<const char*>(data), len); }

    // 以网络字节序写入整数
    void append_int8(int8_t x);
    void append_int16(int16_t x);
    void append_int32(int32_t x);
    void append_int64(int64_t x);

    // 以网络字节序读取整数
    int8_t read_int8();
    int16_t read_int16();
    int32_t read_int32();
    int64_t read_int64();

    int8_t peek_int8() const;
    int16_t peek_int16() const;
    int32_t peek_int32() const;
    int64_t peek_int64() const;

    // 前置操作
    void prepend(const void* data, size_t len);

    // 确保空间
    void ensure_writable_bytes(size_t len);
    void has_written(size_t len) { write_index_ += len; }
    void unwrite(size_t len) { write_index_ -= len; }

    // 搜索操作
    const char* find_crlf() const;
    const char* find_crlf(const char* start) const;
    const char* find_eol() const;
    const char* find_eol(const char* start) const;

    // 输入输出操作
    ssize_t read_fd(int fd, int* saved_errno);
    ssize_t write_fd(int fd, int* saved_errno);

    // 零拷贝操作
    std::vector<struct iovec> get_readable_iovec() const;
    std::vector<struct iovec> get_writable_iovec();

    // 字符串转换
    std::string to_string() const;

    // 交换
    void swap(Buffer& other) noexcept;

private:
    char* begin() { return buffer_.data(); }
    const char* begin() const { return buffer_.data(); }

    void make_space(size_t len);

    std::vector<char> buffer_;
    size_t read_index_;
    size_t write_index_;
};

}  // namespace tzzero::utils







