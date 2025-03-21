#include "InetAddress.h"

#include <strings.h>
#include <string.h>

InetAddress::InetAddress()
{

}

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
};


std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
};
std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    // snprintf(buf+end, 64 + (sizeof end) + (sizeof port), ":%u", port);
    sprintf(buf+end, ":%u", port);

    return buf;
};
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
};

// int main(){
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << std::endl;
//     return 0;
// }

