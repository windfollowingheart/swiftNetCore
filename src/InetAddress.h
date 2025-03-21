#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

class InetAddress
{
public:
    InetAddress();
    explicit InetAddress(uint16_t port, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr) 
    {}

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in *getSocketAddr() const { return &addr_; }
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; };

private:
    struct sockaddr_in addr_;
};