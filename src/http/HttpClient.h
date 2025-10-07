#pragma

#include "../TcpClient.h"
#include "ThreadPool.h"

#include <queue>

class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.

class HttpClient : noncopyable
{
public:
    using HttpResponseCallback = std::function<void(const HttpResponse &)>;

    HttpClient(EventLoop *loop,
               const InetAddress &listenAddr,
               const std::string &name,
               bool useThreadPool = false,
               int maxThreadPoolSize = 0,
               int maxTaskCount = -1);

    EventLoop *getLoop() const { return client_.getLoop(); }

    /// Not thread safe, callback be registered before calling start().
    void setHttpCallback(const HttpResponseCallback &cb)
    {
        httpResponseCallback_ = cb;
    }

    void connect();

    void disconnect();

    void stop();

    // 发送HTTP GET请求
    void get(const std::string &path);

    // 发送HTTP POST请求
    void post(const std::string &path,
              const std::string &body,
              const std::string &contentType);

    // 发送HTTP请求
    void sendRequest(const HttpRequest &req, const HttpResponseCallback &cb);

    void handleResponse(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp receiveTime);

    ThreadPool &getThreadPool()
    {
        return threadPool_;
    }

private:
    void onConnection(const TcpConnectionPtr &conn);
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp receiveTime);

    TcpClient client_;
    HttpResponseCallback httpResponseCallback_;
    bool connected_;
    bool isConnecting_;
    // std::vector<HttpRequest> requests_;
    std::queue<HttpRequest> requests_;
    std::mutex mutex_;
    bool useThreadPool_;
    int maxThreadPoolSize_;
    int maxTaskCount_;
    ThreadPool threadPool_;
};
