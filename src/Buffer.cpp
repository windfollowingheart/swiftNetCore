#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

const char Buffer::kCRLF[] = "\r\n";

/**
 * 从fd上读取数据 Poller工作在LT模式
 * Buffer缓冲区时有大小的，但是从fd上读取数据的时候，却不知道tcp数据最终的大小
 */
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上内存空间
    struct iovec vec[2];
    const size_t writeable = writeableBytes();
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writeable) // buffer的科协缓冲区已经够存储读出的数据
    {
        writeIndex_ += n;
    }
    else
    {
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writeable); // writeIndex 开始写n - writeable大小的数据
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}