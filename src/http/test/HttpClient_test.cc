#include <swiftNetCore/http/HttpClient.h>
#include <swiftNetCore/http/HttpRequest.h>
#include <swiftNetCore/http/HttpResponse.h>
#include <swiftNetCore/EventLoop.h>
#include <swiftNetCore/Logger.h>

#include <iostream>
#include <map>

void onResponse(const HttpResponse &resp)
{
  // std::cout << "Headers " << resp.headers() << std::endl;
  // std::cout << "body " << resp.body() << " status " << resp.statusCode() << std::endl;
  std::cout << " status " << resp.statusCode() << std::endl;

  // 打印所有header
  // for (auto &header : resp.headers())
  // {
  //   std::cout << header.first << " : " << header.second << std::endl;
  // }
}

int main()
{
  Logger &logger = Logger::instance();
  logger.setMinLogLevel(LogLevel::INFO);
  logger.setEnableAsync(true);
  logger.setLogFilename("log/http_client.log");
  logger.startAsyncLogging();
  EventLoop loop;
  HttpClient client(&loop, InetAddress(8989, "127.0.0.1"), "dummy");
  client.setHttpCallback(onResponse);
  // client.connect();
  for (int i = 0; i < 100; i++)
  {
    client.get("/hello");
    client.post("/hello", "{\n  \"name\": \"w\"\n}", "application/json");
  }
  client.get("/hello");
  client.get("/hello");

  loop.loop();
}
