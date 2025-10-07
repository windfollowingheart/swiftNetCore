#include "Logger.h"
#include "Timestamp.h"
#include <iostream>

// 获取日志唯一的实例对象
Logger &Logger::instance()
{
    static Logger logger;
    return logger;
};
// 设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
};
// 写日志 [级别信息] time : msg
void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
    case INFO:
        if (minLogLevel_ <= INFO)
        {
            // std::cout << "[INFO]" << minLogLevel_;
            msg = Timestamp::now().toString() + " [INFO]: " + msg;
        }
        break;
    case ERROR:
        if (minLogLevel_ <= ERROR)
        {
            // std::cout << "[ERROR]";
            msg = Timestamp::now().toString() + " [ERROR]: " + msg;
        }
        break;
    case FATAL:
        if (minLogLevel_ <= FATAL)
        {
            // std::cout << "[FATAL]";
            msg = Timestamp::now().toString() + " [FATAL]: " + msg;
        }
        break;
    case DEBUG:
        if (minLogLevel_ <= DEBUG)
        {
            // std::cout << "[DEBUG]" << minLogLevel_;
            msg = Timestamp::now().toString() + " [DEBUG]: " + msg;
        }
        break;
    case WARN:
        if (minLogLevel_ <= WARN)
        {
            // std::cout << "[WARN]";
            msg = Timestamp::now().toString() + " [WARN]: " + msg;
        }
        break;

    default:
        break;
    }

    // 打印时间和msg
    if (minLogLevel_ <= logLevel_)
    {
        // std::cout < < < < " : " << msg << std::endl;
        if (enableAsync_)
        {
            asyncLogger_->log(msg);
        }
        else
        {
            std::cout << msg << std::endl;
        }
    }
};
