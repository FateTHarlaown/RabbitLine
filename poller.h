//
// Created by NorSnow_ZJ on 2018/7/27.
//

#ifndef RABBITLINE_POLLER_H
#define RABBITLINE_POLLER_H

#include <poll.h>
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include "callbacks.h"
#include "timers.h"

namespace RabbitLine
{

class Channel;

class Poller;

extern Poller *getLocalPoller();

//给协程库用的poller都是在一个线程用的，不用考虑并发
class Poller
{
public:
    using ChannelList = std::vector<Channel *>;
    using TimerPtr = std::shared_ptr<Timer>;
    using TimerList = std::vector<std::pair<Timestamp, TimerPtr>>;
    using TimerTree = std::multimap<Timestamp, TimerPtr>;
    using TimerMap = std::unordered_map<int64_t, TimerPtr>;
    using FunctorList = std::vector<PendingCallbackFunc>;

    Poller() : seq_(0)
    {

    }
    virtual ~Poller();

    Poller(const Poller &) = delete;
    const Poller &operator=(const Poller &) = delete;

    virtual void runPoll() = 0;
    virtual void addChannel(Channel *ch) = 0;
    virtual void removeChannel(Channel *ch) = 0;
    virtual void updateChannel(Channel *ch) = 0;

    int64_t addTimer(Timestamp expirationTime, TimeoutCallbackFunc callback, bool repeat = false, int interval = 0);
    void removeTimer(int64_t seq);
    void addPendingFunction(PendingCallbackFunc func);

protected:
    virtual void getActiveChannels() = 0;
    virtual void handleEvents();

    void getExpiredTimers();
    void dealExpiredTimers();
    void dealPendingFunctors();

protected:
    //100 ms for debug
    static const int kIntervalTime = 100;
    int64_t seq_;
    ChannelList activeChannels_;
    TimerTree timers_;
    TimerList timeOutQue_;
    TimerMap waitingTimers_;
    FunctorList pendingFunctors_;
};

class PollPoller : public Poller
{
public:
    using EventList = std::vector<struct pollfd>;
    using ChannelMap = std::unordered_map<int, Channel *>;

    virtual void runPoll();
    virtual void addChannel(Channel *ch);
    virtual void removeChannel(Channel *ch);
    virtual void updateChannel(Channel *ch)
    {};

protected:
    virtual void getActiveChannels();

private:
    void preparePollEvents();

private:
    EventList pollfds_;
    //channel的生命周期由外部管理
    ChannelMap pollChannels_;
};

//todo:finish it
class EpollPoller : public Poller
{
public:
    using EventList = std::vector<struct epoll_event>;
    EpollPoller();
    virtual ~EpollPoller();

    virtual void runPoll();
    virtual void addChannel(Channel *ch);
    virtual void removeChannel(Channel *ch);
    virtual void updateChannel(Channel *ch);

protected:
    virtual void getActiveChannels();

private:
    const static int kAddOperation = EPOLL_CTL_ADD;
    const static int kDelOperation = EPOLL_CTL_DEL;
    const static int kModOperation = EPOLL_CTL_MOD;
    const static int kInitEnventNum = 255;

    int epollFd_;
    int activeChannelInThisTurn_;
    EventList eventList_;
};

}

#endif //RABBITLINE_POLLER_H

