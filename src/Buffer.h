#pragma once

#include <vector>
#include <string>
#include <algorithm>

// 网络库底层的缓冲区类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writeIndex_(kCheapPrepend)
    {
    }

    size_t readableBytes() const { return writeIndex_ - readerIndex_; }

    size_t writeableBytes() const { return buffer_.size() - writeIndex_; }

    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区可读数据的起始地址
    const char *peek() const { return begin() + readerIndex_; }

    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len; // 说明只读取了可读缓冲区数据的一部分，还剩下readerIndex_
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writeIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的buffer数据转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 上面一句把缓冲区中可读的数据已经读取出来，这里对缓冲区进行复位操作
        return result;
    }

    void ensureWriteableBytes(size_t len)
    {
        if (writeableBytes() < len)
        {
            makeSpace(len);
        }
    }

    // 把[data, data+len]内存的数据添加到writeable缓冲区中
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writeIndex_ += len;
    }

    char *beginWrite()
    {
        return begin() + writeIndex_;
    }

    const char *beginWrite() const
    {
        return begin() + writeIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char *begin()
    {
        // it.operator*()
        return &*buffer_.begin();
    }
    const char *begin() const
    {
        // it.operator*()
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        if (writeableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writeIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writeIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writeIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writeIndex_;
};
