// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "TcpClient.h"

#include "Logger.h"
#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <stdio.h> // snprintf
#include <functional>

// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

namespace detail
{
  void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn)
  {
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  }

  void removeConnector(const ConnectorPtr &connector)
  {
    // connector->
  }
}

TcpClient::TcpClient(EventLoop *loop,
                     const InetAddress &serverAddr,
                     const std::string &nameArg)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr)),
      name_(nameArg),
      connectionCallback_(),
      messageCallback_(),
      retry_(false),
      connect_(true),
      nextConnId_(1)
{
  connector_->setNewConnectionCallback(
      std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
  // FIXME setConnectFailedCallback
  LOG_INFO("TcpClient::TcpClient[%s] - connector %p", name_.c_str(), get_pointer(connector_));
}

TcpClient::~TcpClient()
{
  LOG_INFO("TcpClient::~TcpClient[%s] - connector %p", name_.c_str(), get_pointer(connector_));
  TcpConnectionPtr conn;
  bool unique = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }
  if (conn)
  {
    // FIXME: not 100% safe, if we are in different thread
    CloseCallback cb = std::bind(&detail::removeConnection, loop_, std::placeholders::_1);
    loop_->runInLoop(
        std::bind(&TcpConnection::setCloseCallback, conn, cb));
    if (unique)
    {
    }
  }
  else
  {
    connector_->stop();
    // FIXME: HACK
    loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
  }
}

void TcpClient::connect()
{
  // FIXME: check state
  LOG_INFO("TcpClient::connect[%s] - connecting to %s", name_.c_str(), connector_->serverAddress().toIpPort().c_str());
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect()
{
  connect_ = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_)
    {
      connection_->shutdown();
    }
  }
}

void TcpClient::stop()
{
  connect_ = false;
  connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
  InetAddress peerAddr(sockets::getPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;

  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));

  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      std::bind(&TcpClient::removeConnection, this, conn)); // FIXME: unsafe
  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_ = conn;
  }
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_.reset();
  }

  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_)
  {
    LOG_INFO("TcpClient::connect[%s] - Reconnecting to %s", name_.c_str(), connector_->serverAddress().toIpPort().c_str());
    connector_->restart();
  }
}
