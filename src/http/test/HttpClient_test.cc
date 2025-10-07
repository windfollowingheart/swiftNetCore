#include <swiftNetCore/http/HttpClient.h>
#include <swiftNetCore/http/HttpRequest.h>
#include <swiftNetCore/http/HttpResponse.h>
#include <swiftNetCore/EventLoop.h>
#include <swiftNetCore/Logger.h>
#include <swiftNetCore/http/ThreadPool.h>

#include <iostream>
#include <map>
#include <mutex>

static std::mutex mtx;
static long long c = 0;
static ThreadPool threadPool(100);

void onResponse(const HttpResponse &resp)
{
  // std::cout << "Headers " << resp.headers() << std::endl;
  // std::cout << "body " << resp.body() << " status " << resp.statusCode() << std::endl;

  // LOG_WARN(" status %d", resp.statusCode());
  // usleep(100000);

  // std::lock_guard<std::mutex> lock(mtx);
  threadPool.submit([&]()
                    { 
                      std::lock_guard<std::mutex> lock(mtx);
                      c++; });

  // 打印所有header
  // for (auto &header : resp.headers())
  // {
  //   std::cout << header.first << " : " << header.second << std::endl;
  // }
}

void monitorThread(HttpClient &client)
{
  while (true)
  {
    int threadCount = client.getThreadPool().threadsNum();
    int taskCount = client.getThreadPool().tasksNum();
    LOG_WARN("monitor thread, threadCount %d, taskCount %d", threadCount, taskCount);
    // usleep(1000000);
    LOG_WARN("c %d", c);
    this_thread::sleep_for(chrono::seconds(1));
  }
}

int main()
{
  Logger &logger = Logger::instance();
  logger.setMinLogLevel(LogLevel::WARN);
  logger.setEnableAsync(true);
  logger.setLogFilename("log/http_client.log");
  logger.startAsyncLogging();
  EventLoop loop;
  HttpClient client(&loop, InetAddress(8989, "127.0.0.1"), "dummy");
  client.setHttpCallback(onResponse);
  // client.connect();
  for (int i = 0; i < 10000; i++)
  {
    client.get("/hello");
    client.post("/hello", "{\n  \"name\": \"w\"\n}", "application/json");
  }
  // client.get("/hello");
  // client.get("/hello");

  std::thread monitor(monitorThread, std::ref(client));
  // monitor.join();

  loop.loop();
}
