
#include "Poller.h"

Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop)
{

}

bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    if(it != channels_.end() && it->second == channel){
        return true;
    }else { 
        return false; 
    }
}

// Poller* Poller::newDefaultPoller(EventLoop *loop) 
// {

// }
