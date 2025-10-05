#include <swiftNetCore/TcpServer.h>
#include <swiftNetCore/Logger.h>
#include <swiftNetCore/TcpClient.h>
#include <swiftNetCore/SocketsOps.h>

#include <iostream>
#include <stdio.h>
#include <memory>
#include <unistd.h>

void send(TcpConnectionPtr conn,
          std::string &message)
{
    conn->send(message);
}

class ChatClient
{
public:
    ChatClient(EventLoop *loop, const InetAddress &serverAddr)
        : client_(loop, serverAddr, "ChatClient")
    {
        client_.setConnectionCallback(
            std::bind(&ChatClient::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&ChatClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        client_.enableRetry();
    }

    ~ChatClient()
    {
        client_.disconnect();
    }

    void connect()
    {
        client_.connect();
    }

    void disconnect()
    {
        client_.disconnect();
    }

    void write(std::string &message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_)
        {
            send(connection_, message);
        }
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        LOG_INFO("ChatClient::onConnection - %s -> %s is %s",
                 conn->localAddress().toIpPort().c_str(),
                 conn->peerAddress().toIpPort().c_str(),
                 (conn->connected() ? "UP" : "DOWN"));

        std::lock_guard<std::mutex> lock(mutex_);
        if (conn->connected())
        {
            connection_ = conn;

            std::string msg = "Hello, server!";
            send(connection_, msg);
        }
        else
        {
            printf("hhh\n");
            connection_.reset();
        }
    }

    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        // conn->send(msg);
        printf("<<< %s\n", msg.c_str());
        // conn->shutdown();
        client_.disconnect();
        // delete this;
        // loop_->quit();
    }

    EventLoop *loop_;
    TcpClient client_;
    std::mutex mutex_;
    TcpConnectionPtr connection_;
};

int main()
{
    EventLoop loop;
    uint16_t port = 9999;
    InetAddress serverAddr(port, "127.0.0.1");

    ChatClient client(&loop, serverAddr);
    client.connect();

    loop.loop();
    // std::string line;
    // while (std::getline(std::cin, line))
    // {
    //     client.write(line);
    // }
    // client.disconnect();
}