#include "tzzero/utils/logger.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <filesystem>

namespace tzzero::utils {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::string log_line = "[" + get_timestamp() + "] "
                          + "[" + level_to_string(level) + "] "
                          + message;
    
    // 始终输出到控制台
    std::cout << log_line << std::endl;
    
    // 如果配置了则输出到文件
    if (!output_file_.empty() && file_stream_.is_open()) {
        file_stream_ << log_line << std::endl;
        file_stream_.flush();
        current_file_size_ += log_line.length() + 1;  // +1 为换行符
        
        if (current_file_size_ >= max_file_size_) {
            rotate_log_file();
        }
    }
}

void Logger::log(LogLevel level, const char* file, int line, const std::string& message) {
    if (level < level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::string log_line = "[" + get_timestamp() + "] "
                          + "[" + level_to_string(level) + "] "
                          + "[" + file + ":" + std::to_string(line) + "] "
                          + message;
    
    // 始终输出到控制台
    std::cout << log_line << std::endl;
    
    // 如果配置了则输出到文件
    if (!output_file_.empty() && file_stream_.is_open()) {
        file_stream_ << log_line << std::endl;
        file_stream_.flush();
        current_file_size_ += log_line.length() + 1;  // +1 为换行符
        
        if (current_file_size_ >= max_file_size_) {
            rotate_log_file();
        }
    }
}

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKN ";
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Logger::set_output_file(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    
    output_file_ = filename;
    current_file_size_ = 0;
    
    if (!filename.empty()) {
        file_stream_.open(filename, std::ios::app);
        if (file_stream_.is_open()) {
            // 获取当前文件大小
            file_stream_.seekp(0, std::ios::end);
            current_file_size_ = file_stream_.tellp();
            file_stream_.seekp(0, std::ios::beg);
        }
    }
}

void Logger::rotate_log_file() {
    if (output_file_.empty()) return;
    
    file_stream_.close();
    
    // 轮转现有的日志文件
    for (int i = max_files_ - 1; i > 0; --i) {
        std::string old_file = output_file_ + "." + std::to_string(i);
        std::string new_file = output_file_ + "." + std::to_string(i + 1);
        
        if (std::filesystem::exists(old_file)) {
            if (i == max_files_ - 1) {
                std::filesystem::remove(old_file);  // 删除最旧的
            } else {
                std::filesystem::rename(old_file, new_file);
            }
        }
    }
    
    // 将当前文件移动到 .1
    if (std::filesystem::exists(output_file_)) {
        std::filesystem::rename(output_file_, output_file_ + ".1");
    }
    
    // 创建新的日志文件
    file_stream_.open(output_file_, std::ios::trunc);
    current_file_size_ = 0;
}

}  // namespace tzzero::utils
