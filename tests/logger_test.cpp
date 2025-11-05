#include <gtest/gtest.h>
#include "tzzero/utils/logger.h"
#include <fstream>
#include <filesystem>

using namespace tzzero::utils;

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::instance().set_level(LogLevel::DEBUG);
    }

    void TearDown() override {
        // 清理测试日志文件
        if (std::filesystem::exists("test_log.txt")) {
            std::filesystem::remove("test_log.txt");
        }
    }
};

TEST_F(LoggerTest, Singleton) {
    Logger& logger1 = Logger::instance();
    Logger& logger2 = Logger::instance();

    EXPECT_EQ(&logger1, &logger2);
}

TEST_F(LoggerTest, SetAndGetLevel) {
    Logger::instance().set_level(LogLevel::WARN);
    EXPECT_EQ(Logger::instance().get_level(), LogLevel::WARN);

    Logger::instance().set_level(LogLevel::DEBUG);
    EXPECT_EQ(Logger::instance().get_level(), LogLevel::DEBUG);
}

TEST_F(LoggerTest, BasicLogging) {
    // 测试基本的日志记录功能
    EXPECT_NO_THROW({
        Logger::instance().log(LogLevel::INFO, "Test message");
        Logger::instance().log(LogLevel::ERROR, __FILE__, __LINE__, "Error message");
    });
}

TEST_F(LoggerTest, LogLevelFiltering) {
    Logger::instance().set_level(LogLevel::WARN);

    // DEBUG 和 INFO 不应该被记录
    // WARN, ERROR, FATAL 应该被记录
    EXPECT_NO_THROW({
        Logger::instance().log(LogLevel::DEBUG, "Debug message");
        Logger::instance().log(LogLevel::INFO, "Info message");
        Logger::instance().log(LogLevel::WARN, "Warn message");
        Logger::instance().log(LogLevel::ERROR, "Error message");
    });
}

TEST_F(LoggerTest, MacroUsage) {
    // 测试日志宏
    EXPECT_NO_THROW({
        LOG_DEBUG("Debug message using macro");
        LOG_INFO("Info message using macro");
        LOG_WARN("Warning message using macro");
        LOG_ERROR("Error message using macro");
    });
}

TEST_F(LoggerTest, FileOutput) {
    const std::string log_file = "test_log.txt";
    Logger::instance().set_output_file(log_file);

    LOG_INFO("Test file output");

    // 确保文件被创建
    EXPECT_TRUE(std::filesystem::exists(log_file));
}

TEST_F(LoggerTest, StreamStyleLogging) {
    // 测试流式风格的日志记录
    EXPECT_NO_THROW({
        LOG_INFO("Integer: " << 42 << ", String: " << "test" << ", Float: " << 3.14);
    });
}
