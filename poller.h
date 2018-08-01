//
// Created by NorSnow_ZJ on 2018/7/27.
//

#ifndef COROUTLINE_POLLER_H
#define COROUTLINE_POLLER_H

#include <poll.h>
#include <functional>
#include <vector>
#include <unordered_map>

using EventCallbackFunc = std::function<void ()>;
using TimeoutCallbackFunc = std::function<void ()>;

class poller;

class Channel
{
public:
    Channel(poller* po, int event) : poller_(po), events_()
    {
    }

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    int getEvents()
    {
        return events_;
    }

    void setRevents(int revents)
    {
        revents_ = revents;
    }

    void enableWirte()
    {
        events_ |= kWriteEvent;
    }

    void enableRead()
    {
        events_ |= kReadEvent;
    }

    void disableWrite()
    {
        events_ &= ~kWriteEvent;
    }

    void disableRead()
    {
        events_ &= ~kReadEvent;
    }

    int getFd()
    {
        return fd_;
    }

    void handleEvents();

private:
    const static int kNoEvent = 0;
#ifdef  __CYGWIN__
    const static int kReadEvent = POLLIN | POLLPRI;
    const static int kWriteEvent = POLLOUT;
    const static int kErrorEvent = POLLERR;
#else
    //todo:linux 上使用Eepoll来实现
    const static int kReadEvent = EPOLLIN;
    const static int kWriteEvent = EPOLLOUT;
#endif
    poller * poller_;
    int events_;
    int revents_;
    int fd_;
    EventCallbackFunc readCallbackFunc_;
    EventCallbackFunc writeCallbackFunc_;
    EventCallbackFunc errorCallbackFunc_;
};
//给协程库用的poller都是在一个线程用的，不用考虑并发
class poller
{
public:
    virtual void poll() = 0;
    virtual void addChannel(Channel* ch) = 0;
    virtual void removeChannel(Channel* ch) = 0;

protected:
    std::vector<Channel*> activeChannels_;
    std::vector<TimeoutCallbackFunc> timeOutQue_;
    static const int kIntervalTime = 1;
};

class PollPoller : public poller
{
public:
    virtual void poll();
    virtual void handleEvents();
    virtual void addChannel(Channel* ch)
    {
        pollChannels_.insert(std::pair<int,Channel*>(ch->getFd(), ch));
    }

    virtual void removeChannel(Channel* ch)
    {
        if (pollChannels_.end() != pollChannels_.find(ch->getFd())) {
            pollChannels_.erase(ch->getFd());
        }
    }

private:
    std::vector<struct pollfd> pollfds_;
    std::unordered_map<int, Channel*> pollChannels_;
};

//tood:finish it
class EpollPoller : public poller
{
public:

private:
};




#endif //COROUTLINE_POLLER_H

