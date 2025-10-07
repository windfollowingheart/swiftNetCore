#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>
#include <sstream>
#include <queue>

#include "Timestamp.h"

// 高性能异步日志类
class AsyncLogger
{
public:
    AsyncLogger(const std::string &filename, size_t buffer_size = 2 * 1024 * 1024)
        : log_file_(filename, std::ios::app),
          buffer_size_(buffer_size),
          running_(true)
    {

        if (!log_file_.is_open())
        {
            throw std::runtime_error("Failed to open log file");
        }

        current_buffer_.reserve(buffer_size_);
        write_thread_ = std::thread(&AsyncLogger::writeLoop, this);
    }

    ~AsyncLogger()
    {
        stop();
    }

    void stop()
    {
        if (running_.exchange(false))
        {
            cond_.notify_all();
            if (write_thread_.joinable())
            {
                write_thread_.join();
            }

            // 写入剩余日志
            std::lock_guard<std::mutex> lock(mutex_);
            if (!current_buffer_.empty())
            {
                writeToFile(current_buffer_);
            }
            while (!buffers_to_write_.empty())
            {
                writeToFile(buffers_to_write_.front());
                buffers_to_write_.pop();
            }

            if (log_file_.is_open())
            {
                log_file_.close();
            }
        }
    }

    void log(const std::string &message)
    {
        std::string log_entry = message + "\n";

        std::lock_guard<std::mutex> lock(mutex_);

        if (current_buffer_.capacity() - current_buffer_.size() < log_entry.size())
        {
            // 当前缓冲区已满，交换到待写入队列
            buffers_to_write_.push(std::move(current_buffer_));
            current_buffer_.clear();
            current_buffer_.reserve(buffer_size_);
            cond_.notify_one();
        }

        current_buffer_ += log_entry;
    }

private:
    void writeLoop()
    {
        std::vector<std::string> local_buffers;
        local_buffers.reserve(16);

        while (running_ || !buffers_to_write_.empty() || !current_buffer_.empty())
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);

                if (buffers_to_write_.empty() && current_buffer_.empty())
                {
                    cond_.wait_for(lock, std::chrono::milliseconds(100));
                    continue;
                }

                // 将当前缓冲区移到待写入队列
                if (!current_buffer_.empty())
                {
                    buffers_to_write_.push(std::move(current_buffer_));
                    current_buffer_.clear();
                    current_buffer_.reserve(buffer_size_);
                }

                // 批量交换缓冲区
                while (!buffers_to_write_.empty() && local_buffers.size() < 16)
                {
                    local_buffers.push_back(std::move(buffers_to_write_.front()));
                    buffers_to_write_.pop();
                }
            }

            // 写入文件
            for (auto &buffer : local_buffers)
            {
                if (!buffer.empty())
                {
                    // 打印写入的日志内容
                    std::cout << buffer << std::endl;
                    writeToFile(buffer);
                }
            }
            local_buffers.clear();
        }
    }

    void writeToFile(const std::string &buffer)
    {
        log_file_ << buffer;
        log_file_.flush(); // 确保数据写入磁盘
    }

    const size_t buffer_size_;
    std::ofstream log_file_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> running_;
    std::thread write_thread_;

    std::string current_buffer_;
    std::queue<std::string> buffers_to_write_;
};
