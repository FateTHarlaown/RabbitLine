//
// Created by NorSnow_ZJ on 2018/7/27.
//

#include <cassert>
#include <iostream>
#include <unistd.h>
#include "poller.h"
#include "channel.h"

using namespace RabbitLine;

__thread Poller * localPoller = NULL;

Poller * RabbitLine::getLocalPoller()
{
    if (!localPoller) {
#ifdef __linux__
       localPoller = new EpollPoller();
#else
        localPoller = new PollPoller();
#endif
    }

    return localPoller;
}

int64_t Poller::addTimer(Timestamp expirationTime, TimeoutCallbackFunc callback, bool repeat, int interval)
{
    if (expirationTime < Timestamp::now()) {
        return -1;
    }

    int64_t timerSeq = seq_++;
    TimerPtr timer = std::make_shared<Timer>(expirationTime, callback, timerSeq, repeat, interval);
    waitingTimers_.insert(TimerMap::value_type(timerSeq, timer));
    timers_.insert(TimerTree::value_type(expirationTime, timer));

    return timerSeq;
}

void Poller::removeTimer(int64_t seq)
{
    auto it = waitingTimers_.find(seq);
    if (it == waitingTimers_.end()) {
        return;
    }

    TimerPtr delTimer = it->second;
    size_t count = timers_.count(delTimer->getExpiration());
    assert(count > 0);

    auto t = timers_.find(delTimer->getExpiration());
    for (size_t i = 0; i < count; i++, t++) {
        if (it->second == delTimer) {
            timers_.erase(t);
            waitingTimers_.erase(seq);
            break;
        }
    }
}

Poller::~Poller()
{
}

void Poller::handleEvents()
{
    for (const auto& c: activeChannels_) {
        c->handleEvents();
    }
}

void Poller::getExpiredTimers()
{
    timeOutQue_.clear();
    if (!timers_.empty()) {
        Timestamp now = Timestamp::now();
        auto bound = timers_.lower_bound(now);
        std::copy(timers_.begin(), bound, std::back_inserter(timeOutQue_));
        timers_.erase(timers_.begin(), bound);
    }
}

void Poller::dealExpiredTimers()
{
    for (const auto& t: timeOutQue_) {
        /*这个必须在run的前面！*/
        if (!t.second->isRepeat()) {
            waitingTimers_.erase(t.second->getTimerid());
        }

        t.second->run();

        if (t.second->isRepeat()) {
            t.second->reset();
            timers_.insert(TimerTree::value_type(t.second->getExpiration(), t.second));
        }
    }
}

void Poller::dealPendingFunctors()
{
    for (auto& f : pendingFunctors_) {
        f();
    }
}

void Poller::addPendingFunction(PendingCallbackFunc func)
{
    pendingFunctors_.push_back(func);
}


void PollPoller::runPoll()
{
    //准备poll需要的监听事件
    preparePollEvents();
    int ret = poll(pollfds_.data(), pollfds_.size(), kIntervalTime);
    if (ret < 0) {
        perror("poll error");
        //std::cout << "ret is " << ret << std::endl;
    }

    //提取有事件发生的Channel
    getActiveChannels();
    //处理channel上的事件
    handleEvents();
    //提取到期的定时器事件
    getExpiredTimers();
    //处理定时器事件
    dealExpiredTimers();
    //处理挂起的任务
    dealPendingFunctors();
}

void PollPoller::addChannel(Channel *ch)
{
    if (pollChannels_.end() == pollChannels_.find(ch->getFd()) ) {
        pollChannels_.insert(ChannelMap::value_type(ch->getFd(), ch));
    }
}

void PollPoller::removeChannel(Channel *ch)
{
    if (pollChannels_.end() != pollChannels_.find(ch->getFd())) {
        pollChannels_.erase(ch->getFd());
    }

}

void PollPoller::preparePollEvents()
{
    pollfds_.clear();
    for (const auto& p : pollChannels_) {
        struct pollfd ev;
        ev.fd = p.first;
        ev.events = (short)p.second->getEvents();
        pollfds_.push_back(ev);
        p.second->setRevents(Channel::kNoEvent);
    }
}

void PollPoller::getActiveChannels()
{
    activeChannels_.clear();
    for (const auto & ev : pollfds_) {
        if (ev.revents & (POLLOUT | POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL)) {
            if (pollChannels_.find(ev.fd) == pollChannels_.end()) {
                continue;
            }

            Channel * ch = pollChannels_[ev.fd];
            ch->setRevents(ev.revents);
            activeChannels_.push_back(ch);
        }
    }
}

EpollPoller::EpollPoller() : activeChannelInThisTurn_(0)
{
    int epollfd = epoll_create(5);
    if (epollfd < 0) {
        perror("epoll create failed!");
        abort();
    }
    epollFd_ = epollfd;
    eventList_.resize(kInitEnventNum);
}

EpollPoller::~EpollPoller()
{

}

void EpollPoller::runPoll()
{
    int ret = epoll_wait(epollFd_, &(*eventList_.begin()), eventList_.size(), kIntervalTime);
    //std::cout << "epoll trun!" << std::endl;
    if (ret == -1) {
        perror("epoll_wait error!");
        return;
    }
    activeChannelInThisTurn_ = ret;
    //提取有事件发生的Channel
    getActiveChannels();
    //处理channel上的事件
    handleEvents();
    //提取到期的定时器事件
    getExpiredTimers();
    //处理定时器事件
    dealExpiredTimers();
    //处理挂起的任务
    dealPendingFunctors();

    if (eventList_.size() == activeChannelInThisTurn_) {
        eventList_.resize(2*activeChannelInThisTurn_);
    }
}

void EpollPoller::addChannel(Channel *ch)
{
    if (!ch->isAddedToPoller()) {
        struct epoll_event event;
        event.data.ptr = static_cast<void*>(ch);
        event.events = ch->getEvents();
        epoll_ctl(epollFd_, kAddOperation, ch->getFd(), &event);
    }
}

void EpollPoller::removeChannel(Channel *ch)
{
    if (ch->isAddedToPoller()) {
        struct epoll_event event;
        event.data.ptr = static_cast<void*>(ch);
        event.events = ch->getEvents();
        epoll_ctl(epollFd_, kModOperation, ch->getFd(), &event);
    }
}

void EpollPoller::updateChannel(Channel *ch)
{
    if (ch->isAddedToPoller()) {
        struct epoll_event event;
        event.data.ptr = static_cast<void*>(ch);
        event.events = ch->getEvents();
        epoll_ctl(epollFd_, kDelOperation, ch->getFd(), &event);
    }
}

void EpollPoller::getActiveChannels()
{
    activeChannels_.clear();
    for (int i = 0; i < activeChannelInThisTurn_; i++) {
        Channel * ch = static_cast<Channel*>(eventList_[i].data.ptr);
        ch->setRevents(eventList_[i].events);
        activeChannels_.push_back(ch);
    }
}

