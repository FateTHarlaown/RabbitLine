//
// Created by NorSnow_ZJ on 2018/7/27.
//

#ifndef COROUTLINE_POLLER_H
#define COROUTLINE_POLLER_H

#include <poll.h>
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <vector>
#include <map>
#include <unordered_map>
#include "callbacks.h"
#include "timers.h"

class Poller;

class Channel
{
public:
    Channel(Poller* po, int fd) : poller_(po), fd_(fd), events_(kNoEvent)
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

    void clearEvents()
    {
        events_ = kNoEvent;
    }

    int getFd()
    {
        return fd_;
    }

    void addToPoller()
    {
        //poller_->addChannel(this);
    }

    void removeFromPoller()
    {
        //poller_->removeChannel(this);
    }

    void handleEvents();

public:
    const static int kNoEvent = 0;
#ifdef  __linux__
    const static int kReadEvent = EPOLLIN | EPOLLPRI;
    const static int kWriteEvent = EPOLLOUT;
    const static int kErrorEvent = EPOLLERR;
#else
    const static int kReadEvent = POLLIN | POLLPRI;
    const static int kWriteEvent = POLLOUT;
    const static int kErrorEvent = POLLERR;
#endif

private:
    Poller * poller_;
    int events_;
    int revents_;
    int fd_;
    EventCallbackFunc readCallbackFunc_;
    EventCallbackFunc writeCallbackFunc_;
    EventCallbackFunc errorCallbackFunc_;
};

//给协程库用的poller都是在一个线程用的，不用考虑并发
class Poller
{
public:
    Poller() : seq_(0)
    {

    }
    Poller(const Poller&) = delete;
    const Poller&operator=(const Poller&) = delete;
    virtual void runPoll() = 0;
    virtual void addChannel(Channel* ch) = 0;
    virtual void removeChannel(Channel* ch) = 0;
    virtual ~Poller();
    int64_t addTimer(Timestamp expirationTime, TimeoutCallbackFunc callback, bool repeat = false, int interval = 0);
    void removeTimer(int64_t seq);
    void addPendingFunction(PendingCallbackFunc func);

protected:
    void getExpiredTimers();
    void dealExpiredTimers();
    void dealPendingFunctors();

protected:
    static const int kIntervalTime = 1;
    int64_t seq_;
    std::vector<Channel*> activeChannels_;
    std::multimap<Timestamp, Timer*> timers_;
    std::vector<std::pair<Timestamp, Timer*>> timeOutQue_;
    std::unordered_map<int64_t , Timer*> waitingTimers_;
    std::vector<PendingCallbackFunc> pendingFunctors_;
};

class PollPoller : public Poller
{
public:
    virtual void runPoll();
    virtual void addChannel(Channel* ch);
    virtual void removeChannel(Channel* ch);

private:
    void getActiveChannels();
    void preparePollEvents();
    void handleEvents();

private:
    std::vector<struct pollfd> pollfds_;
    //channel的生命周期由外部管理
    std::unordered_map<int, Channel*> pollChannels_;
};

//todo:finish it
class EpollPoller : public Poller
{
public:

private:
};




#endif //COROUTLINE_POLLER_H

