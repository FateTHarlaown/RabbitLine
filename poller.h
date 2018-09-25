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
#include <unordered_map>
#include "callbacks.h"
#include "timers.h"

namespace RabbitLine
{

class Channel;

class Poller;

Poller *getLocalPoller();

//给协程库用的poller都是在一个线程用的，不用考虑并发
class Poller
{
public:
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
    //1000 ms for debug
    static const int kIntervalTime = 1000;
    int64_t seq_;
    std::vector<Channel *> activeChannels_;
    std::multimap<Timestamp, Timer *> timers_;
    std::vector<std::pair<Timestamp, Timer *>> timeOutQue_;
    std::unordered_map<int64_t, Timer *> waitingTimers_;
    std::vector<PendingCallbackFunc> pendingFunctors_;
};

class PollPoller : public Poller
{
public:
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
    std::vector<struct pollfd> pollfds_;
    //channel的生命周期由外部管理
    std::unordered_map<int, Channel *> pollChannels_;
};

//todo:finish it
class EpollPoller : public Poller
{
public:
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
    std::vector<struct epoll_event> eventList_;
};

}

#endif //RABBITLINE_POLLER_H

