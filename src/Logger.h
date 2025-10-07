#pragma once

#include "noncopyable.h"
#include "AsyncLogger.h"
#include <string>

#define MUDEBUG

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_WARN(logmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(WARN);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_ERROR(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FATAL(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...) ;
#endif

// 定义日志级别 DEBUG INFO ERROR FATAL
enum LogLevel
{
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    UNKNOWN,

};

// 输出一个日志类
class Logger : noncopyable
{
public:
    // 获取日志唯一的实例对象
    static Logger &instance();
    // 设置日志级别
    void setLogLevel(int level);
    void setMinLogLevel(int level)
    {
        minLogLevel_ = level;
    };

    void setEnableAsync(bool enable)
    {
        enableAsync_ = enable;
    }

    void setLogFilename(const std::string &filename)
    {
        logFilename_ = filename;
    }

    void startAsyncLogging()
    {
        if (enableAsync_)
        {
            asyncLogger_ = std::make_shared<AsyncLogger>(logFilename_);
        }
    }

    void stopAsyncLogging()
    {
        if (enableAsync_)
        {
            asyncLogger_->stop();
            enableAsync_ = false;
        }
    }

    // 获取日志级别
    int logLevel() { return logLevel_; }
    int minLogLevel() { return minLogLevel_; }
    bool enableAsync() { return enableAsync_; }
    // 写日志
    void log(std::string msg);

private:
    bool enableAsync_;
    int logLevel_;
    int minLogLevel_;
    std::string logFilename_;
    std::shared_ptr<AsyncLogger> asyncLogger_;
    Logger() : minLogLevel_(UNKNOWN), enableAsync_(false) {};
    Logger(bool enableAsync) : minLogLevel_(UNKNOWN), enableAsync_(enableAsync)
    {
        if (enableAsync_)
        {
            asyncLogger_ = std::make_shared<AsyncLogger>("async.log");
        }
    };
};