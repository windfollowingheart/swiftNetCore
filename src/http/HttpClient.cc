#include "HttpClient.h"
#include "../Logger.h"

#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include <iostream>

namespace detail
{

    void defaultHttpCallback(const HttpResponse &resp)
    {
    }

} // namespace detail

HttpClient::HttpClient(EventLoop *loop,
                       const InetAddress &serverAddr,
                       const std::string &name,
                       bool useThreadPool,
                       int maxThreadPoolSize,
                       int maxTaskCount)
    : client_(loop, serverAddr, name),
      httpResponseCallback_(detail::defaultHttpCallback),
      connected_(false), isConnecting_(false),
      useThreadPool_(useThreadPool),
      maxThreadPoolSize_(maxThreadPoolSize),
      maxTaskCount_(maxTaskCount)
{
    if (useThreadPool_)
    {
        threadPool_.setMaxThreads(maxThreadPoolSize_);
        threadPool_.setMaxTasks(maxTaskCount_);
    }

    // 设置连接回调
    client_.setConnectionCallback(
        std::bind(&HttpClient::onConnection, this, std::placeholders::_1));

    // 设置消息回调
    client_.setMessageCallback(
        std::bind(&HttpClient::onMessage, this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));
}

void HttpClient::connect()
{
    client_.connect();
}

void HttpClient::disconnect()
{
    client_.disconnect();
}

void HttpClient::stop()
{
    client_.stop();
}

void HttpClient::get(const std::string &path)
{

    HttpRequest request;
    const char *method = "GET";
    request.setMethod(method, method + strlen(method));
    request.setPath(path.c_str(), path.c_str() + path.size());
    request.setVersion(HttpRequest::kHttp11);

    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ && !isConnecting_)
    {
        LOG_WARN("HttpClient::get - not connected");
        connect();
        isConnecting_ = true;
    }
    if (!connected_)
    {
        // LOG_WARN("HttpClient::get - 添加到请求队列");
        requests_.push(request);
    }
    else
    {
        sendRequest(request, httpResponseCallback_);
    }
}

void HttpClient::post(const std::string &path,
                      const std::string &body,
                      const std::string &contentType)
{
    HttpRequest request;
    const char *method = "POST";
    request.setMethod(method, method + strlen(method));
    request.setPath(path.c_str(), path.c_str() + path.size());
    request.setVersion(HttpRequest::kHttp11);
    request.addHeader("Content-Type", contentType);
    request.addHeader("Content-Length", std::to_string(body.length()));
    request.setBody(body);

    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ && !isConnecting_)
    {
        LOG_WARN("HttpClient::post - not connected");
        connect();
        isConnecting_ = true;
    }
    if (!connected_)
    {
        // LOG_WARN("HttpClient::post - 添加到请求队列");
        requests_.push(request);
    }
    else
    {
        // LOG_WARN("HttpClient::post - 直接发送");
        sendRequest(request, httpResponseCallback_);
    }
}

void HttpClient::sendRequest(const HttpRequest &req, const HttpResponseCallback &cb)
{
    httpResponseCallback_ = cb;

    // 获取连接
    TcpConnectionPtr conn = client_.connection();
    if (conn && conn->connected())
    {
        // 构造HTTP请求
        Buffer buf;
        // 添加请求行
        buf.append(req.methodString());
        buf.append(" ");
        buf.append(req.path());
        buf.append(" HTTP/1.1\r\n");

        // 添加其他头部
        for (const auto &header : req.headers())
        {
            buf.append(header.first);
            buf.append(": ");
            buf.append(header.second);
            buf.append("\r\n");
        }

        // 添加空行
        buf.append("\r\n");

        // 添加请求体
        if (!req.body().empty())
        {
            buf.append(req.body());
        }

        // 发送请求
        conn->send(&buf);
    }
    else
    {
        LOG_WARN("HttpClient::sendRequest - connection is not available");
    }
}

void HttpClient::onConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO("HttpClient::onConnection - %s -> %s is %s",
             conn->localAddress().toIpPort().c_str(),
             conn->peerAddress().toIpPort().c_str(),
             (conn->connected() ? "UP" : "DOWN"));

    if (conn->connected())
    {
        conn->setContext(HttpContext());

        // 发送请求
        std::lock_guard<std::mutex> lock(mutex_);
        connected_ = true;
        // LOG_WARN("HttpClient::onConnection - 发送请求数量 %d", requests_.size());
        if (!requests_.empty())
        {
            // LOG_WARN("HttpClient::onConnection - 发送请求");
            sendRequest(requests_.front(), httpResponseCallback_);
            requests_.pop();
            // 等待10ms
        }
        // 清空请求队列
        // requests_.clear();
    }
    else
    {
        connected_ = false;
        isConnecting_ = false;
    }
}

void HttpClient::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp receiveTime)
{
    handleResponse(conn, buffer, receiveTime);
    // 判断是否还有请求
    std::lock_guard<std::mutex> lock(mutex_);
    if (!requests_.empty())
    {
        HttpRequest req = requests_.front();
        requests_.pop();
        sendRequest(req, httpResponseCallback_);
    }
}

void HttpClient::handleResponse(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp receiveTime)
{
    // HttpContext *context = boost::any_cast<HttpContext>(conn->getMutableContext());
    HttpContext *context = new HttpContext();
    // 解析HTTP响应
    if (context->parseResponse(buffer, receiveTime))
    {
        if (context->gotAll())
        {
            // 调用用户设置的回调函数
            if (httpResponseCallback_)
            {
                // 注意：由于当前HttpContext的设计限制，我们无法直接获取HttpResponse对象
                // 这里需要创建一个临时的HttpResponse对象
                HttpResponse response = context->response(); // 假设连接关闭
                HttpResponse resp = response;
                // 实际应用中应该从context中提取响应信息设置到response对象中
                // 是否使用线程池
                if (useThreadPool_)
                {
                    threadPool_.submit(
                        std::bind(httpResponseCallback_, resp));
                    // LOG_WARN("线程池线程数量 %d, 任务数量 %d", threadPool_.threadsNum(), threadPool_.tasksNum());
                }
                else
                {
                    httpResponseCallback_(response);
                }
                // httpResponseCallback_(response);
            }

            // 重置上下文以准备下一次解析
            context->reset();
        }
    }
    else
    {
        LOG_ERROR("HttpClient::handleResponse - failed to parse response");
        conn->shutdown();
    }
}