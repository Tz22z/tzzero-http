#include "tzzero/utils/buffer.h"
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
#include <algorithm>

namespace tzzero::utils {

Buffer::Buffer(size_t initial_size)
    : buffer_(kCheapPrepend + initial_size)
    , read_index_(kCheapPrepend)
    , write_index_(kCheapPrepend)
{
}

Buffer::Buffer(const Buffer& other)
    : buffer_(other.buffer_)
    , read_index_(other.read_index_)
    , write_index_(other.write_index_)
{
}

Buffer& Buffer::operator=(const Buffer& other) {
    if (this != &other) {
        buffer_ = other.buffer_;
        read_index_ = other.read_index_;
        write_index_ = other.write_index_;
    }
    return *this;
}

Buffer::Buffer(Buffer&& other) noexcept
    : buffer_(std::move(other.buffer_))
    , read_index_(other.read_index_)
    , write_index_(other.write_index_)
{
    other.read_index_ = kCheapPrepend;
    other.write_index_ = kCheapPrepend;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        buffer_ = std::move(other.buffer_);
        read_index_ = other.read_index_;
        write_index_ = other.write_index_;
        other.read_index_ = kCheapPrepend;
        other.write_index_ = kCheapPrepend;
    }
    return *this;
}

void Buffer::retrieve(size_t len) {
    if (len < readable_bytes()) {
        read_index_ += len;
    } else {
        retrieve_all();
    }
}

std::string Buffer::retrieve_as_string(size_t len) {
    std::string result(peek(), len);
    retrieve(len);
    return result;
}

std::string Buffer::retrieve_all_as_string() {
    return retrieve_as_string(readable_bytes());
}

void Buffer::append(const char* data, size_t len) {
    ensure_writable_bytes(len);
    std::copy(data, data + len, begin_write());
    has_written(len);
}

void Buffer::append_int8(int8_t x) {
    append(&x, sizeof(x));
}

void Buffer::append_int16(int16_t x) {
    int16_t be = htons(x);
    append(&be, sizeof(be));
}

void Buffer::append_int32(int32_t x) {
    int32_t be = htonl(x);
    append(&be, sizeof(be));
}

void Buffer::append_int64(int64_t x) {
    int32_t high = static_cast<int32_t>((x >> 32) & 0xFFFFFFFF);
    int32_t low = static_cast<int32_t>(x & 0xFFFFFFFF);
    append_int32(high);
    append_int32(low);
}

int8_t Buffer::read_int8() {
    int8_t result = peek_int8();
    retrieve(sizeof(result));
    return result;
}

int16_t Buffer::read_int16() {
    int16_t result = peek_int16();
    retrieve(sizeof(result));
    return result;
}

int32_t Buffer::read_int32() {
    int32_t result = peek_int32();
    retrieve(sizeof(result));
    return result;
}

int64_t Buffer::read_int64() {
    int64_t result = peek_int64();
    retrieve(sizeof(result));
    return result;
}

int8_t Buffer::peek_int8() const {
    return *reinterpret_cast<const int8_t*>(peek());
}

int16_t Buffer::peek_int16() const {
    int16_t be = *reinterpret_cast<const int16_t*>(peek());
    return ntohs(be);
}

int32_t Buffer::peek_int32() const {
    int32_t be = *reinterpret_cast<const int32_t*>(peek());
    return ntohl(be);
}

int64_t Buffer::peek_int64() const {
    int32_t high = peek_int32();
    int32_t low = peek_int32();
    return (static_cast<int64_t>(high) << 32) | static_cast<uint32_t>(low);
}

void Buffer::prepend(const void* data, size_t len) {
    read_index_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + read_index_);
}

void Buffer::ensure_writable_bytes(size_t len) {
    if (writable_bytes() < len) {
        make_space(len);
    }
}

const char* Buffer::find_crlf() const {
    const char* crlf = std::search(peek(), begin_write(), "\r\n", "\r\n" + 2);
    return crlf == begin_write() ? nullptr : crlf;
}

const char* Buffer::find_crlf(const char* start) const {
    const char* crlf = std::search(start, begin_write(), "\r\n", "\r\n" + 2);
    return crlf == begin_write() ? nullptr : crlf;
}

const char* Buffer::find_eol() const {
    const void* eol = memchr(peek(), '\n', readable_bytes());
    return static_cast<const char*>(eol);
}

const char* Buffer::find_eol(const char* start) const {
    const void* eol = memchr(start, '\n', begin_write() - start);
    return static_cast<const char*>(eol);
}

ssize_t Buffer::read_fd(int fd, int* saved_errno) {
    // Use readv() with stack buffer to avoid frequent reallocation
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writable_bytes();
    
    vec[0].iov_base = begin() + write_index_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    
    if (n < 0) {
        *saved_errno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        write_index_ += n;
    } else {
        write_index_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    
    return n;
}

ssize_t Buffer::write_fd(int fd, int* saved_errno) {
    ssize_t n = ::write(fd, peek(), readable_bytes());
    if (n < 0) {
        *saved_errno = errno;
    } else {
        retrieve(n);
    }
    return n;
}

std::vector<struct iovec> Buffer::get_readable_iovec() const {
    std::vector<struct iovec> iovecs;
    if (readable_bytes() > 0) {
        struct iovec iov;
        iov.iov_base = const_cast<char*>(peek());
        iov.iov_len = readable_bytes();
        iovecs.push_back(iov);
    }
    return iovecs;
}

std::vector<struct iovec> Buffer::get_writable_iovec() {
    std::vector<struct iovec> iovecs;
    if (writable_bytes() > 0) {
        struct iovec iov;
        iov.iov_base = begin_write();
        iov.iov_len = writable_bytes();
        iovecs.push_back(iov);
    }
    return iovecs;
}

std::string Buffer::to_string() const {
    return std::string(peek(), readable_bytes());
}

void Buffer::swap(Buffer& other) noexcept {
    buffer_.swap(other.buffer_);
    std::swap(read_index_, other.read_index_);
    std::swap(write_index_, other.write_index_);
}

void Buffer::make_space(size_t len) {
    if (writable_bytes() + prependable_bytes() < len + kCheapPrepend) {
        // Grow the buffer
        buffer_.resize(write_index_ + len);
    } else {
        // Move readable data to the front
        size_t readable = readable_bytes();
        std::copy(begin() + read_index_, begin() + write_index_, begin() + kCheapPrepend);
        read_index_ = kCheapPrepend;
        write_index_ = read_index_ + readable;
    }
}

} // namespace tzzero::utils
