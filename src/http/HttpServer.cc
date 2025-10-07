

#include "HttpServer.h"

#include "../Logger.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace detail
{

  void defaultHttpCallback(const HttpRequest &, HttpResponse *resp)
  {
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
  }

} // namespace detail

HttpServer::HttpServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &name,
                       TcpServer::Option option)
    : server_(loop, listenAddr, name, option),
      httpCallback_(detail::defaultHttpCallback)
{
  server_.setConnectionCallback(
      std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
  server_.setMessageCallback(
      std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void HttpServer::start()
{
  LOG_WARN("HttpServer[%s] starts listening on %s", server_.name().c_str(), server_.ipPort().c_str());
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn)
{
  if (conn->connected())
  {
    conn->setContext(HttpContext());
  }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp receiveTime)
{
  HttpContext *context = boost::any_cast<HttpContext>(conn->getMutableContext());
  // Buffer buf1 = *buf;
  // string ss = buf1.retrieveAllAsString();
  // LOG_WARN("HttpServer::onMessage - 收到请求 %s", ss.c_str());

  if (!context->parseRequest(buf, receiveTime))
  {
    string msg = "HTTP/1.1 400 Bad Request\r\n\r\n";
    conn->send(msg);
    conn->shutdown();
  }

  if (context->gotAll())
  {
    onRequest(conn, context->request());
    context->reset();
  }
}

void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest &req)
{
  const string &connection = req.getHeader("Connection");
  bool close = connection == "close" ||
               (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
  HttpResponse response(close);
  httpCallback_(req, &response);
  Buffer buf;
  response.appendToBuffer(&buf);
  conn->send(&buf);
  if (response.closeConnection())
  {
    conn->shutdown();
  }
}
