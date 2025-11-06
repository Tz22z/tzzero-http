#include <gtest/gtest.h>
#include "tzzero/utils/buffer.h"
#include <cstring>

using namespace tzzero::utils;

class BufferTest : public ::testing::Test {
protected:
    Buffer buffer;
};

TEST_F(BufferTest, InitialState) {
    EXPECT_EQ(buffer.readable_bytes(), 0);
    EXPECT_GT(buffer.writable_bytes(), 0);
    EXPECT_EQ(buffer.prependable_bytes(), Buffer::kCheapPrepend);
}

TEST_F(BufferTest, AppendAndRetrieve) {
    const std::string data = "Hello, World!";
    buffer.append(data);

    EXPECT_EQ(buffer.readable_bytes(), data.size());
    EXPECT_EQ(std::string(buffer.peek(), data.size()), data);

    std::string retrieved = buffer.retrieve_all_as_string();
    EXPECT_EQ(retrieved, data);
    EXPECT_EQ(buffer.readable_bytes(), 0);
}

TEST_F(BufferTest, AppendMultipleTimes) {
    buffer.append(std::string("Hello"));
    buffer.append(std::string(", "));
    buffer.append(std::string("World!"));

    EXPECT_EQ(buffer.readable_bytes(), 13);
    EXPECT_EQ(buffer.retrieve_all_as_string(), "Hello, World!");
}

TEST_F(BufferTest, PartialRetrieve) {
    buffer.append(std::string("Hello, World!"));

    std::string part1 = buffer.retrieve_as_string(5);
    EXPECT_EQ(part1, "Hello");
    EXPECT_EQ(buffer.readable_bytes(), 8);

    std::string part2 = buffer.retrieve_all_as_string();
    EXPECT_EQ(part2, ", World!");
}

TEST_F(BufferTest, IntegerOperations) {
    buffer.append_int8(127);
    buffer.append_int16(32767);
    buffer.append_int32(2147483647);
    buffer.append_int64(9223372036854775807LL);

    EXPECT_EQ(buffer.read_int8(), 127);
    EXPECT_EQ(buffer.read_int16(), 32767);
    EXPECT_EQ(buffer.read_int32(), 2147483647);
    EXPECT_EQ(buffer.read_int64(), 9223372036854775807LL);
}

TEST_F(BufferTest, PeekIntegerOperations) {
    buffer.append_int32(12345);

    EXPECT_EQ(buffer.peek_int32(), 12345);
    EXPECT_EQ(buffer.readable_bytes(), 4);  // peek 不会消耗数据

    EXPECT_EQ(buffer.read_int32(), 12345);
    EXPECT_EQ(buffer.readable_bytes(), 0);
}

TEST_F(BufferTest, FindCRLF) {
    buffer.append(std::string("Line1\r\nLine2\r\nLine3"));

    const char* crlf = buffer.find_crlf();
    ASSERT_NE(crlf, nullptr);
    EXPECT_EQ(std::string(buffer.peek(), crlf - buffer.peek()), "Line1");
}

TEST_F(BufferTest, PrependOperation) {
    buffer.append(std::string("World"));
    buffer.prepend("Hello ", 6);

    EXPECT_EQ(buffer.retrieve_all_as_string(), "Hello World");
}

TEST_F(BufferTest, EnsureWritableBytes) {
    size_t initial_capacity = buffer.capacity();
    buffer.ensure_writable_bytes(2048);

    EXPECT_GE(buffer.writable_bytes(), 2048);
}

TEST_F(BufferTest, CopyConstructor) {
    buffer.append(std::string("Test Data"));
    Buffer buffer2(buffer);

    EXPECT_EQ(buffer.readable_bytes(), buffer2.readable_bytes());
    EXPECT_EQ(buffer.retrieve_all_as_string(), buffer2.retrieve_all_as_string());
}

TEST_F(BufferTest, MoveConstructor) {
    buffer.append(std::string("Test Data"));
    size_t original_size = buffer.readable_bytes();

    Buffer buffer2(std::move(buffer));

    EXPECT_EQ(buffer2.readable_bytes(), original_size);
    EXPECT_EQ(buffer2.retrieve_all_as_string(), "Test Data");
}

TEST_F(BufferTest, Swap) {
    buffer.append(std::string("Buffer1"));
    Buffer buffer2;
    buffer2.append(std::string("Buffer2"));

    buffer.swap(buffer2);

    EXPECT_EQ(buffer.retrieve_all_as_string(), "Buffer2");
    EXPECT_EQ(buffer2.retrieve_all_as_string(), "Buffer1");
}
