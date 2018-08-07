//
// Created by NorSnow_ZJ on 2018/7/27.
//

#include "poller.h"
#include <inttypes.h>


bool poller::addTimer(Timestamp expirationTime, TimeoutCallbackFunc callback, bool repeat, int interval)
{
    Timer * timer = new Timer(expirationTime, callback, repeat, interval);
    timers_.insert(std::pair<Timestamp, Timer*>(expirationTime, timer));
}

poller::~poller()
{
    for (const auto& p : timers_) {
        if (p.second) {
            delete p.second;
        }
    }
}

void poller::getExpiredTimers()
{
    timeOutQue_.clear();
    if (!timers_.empty()) {
        Timestamp now = Timestamp::now();
        auto bound = timers_.lower_bound(now);
        std::copy(timers_.begin(), bound, std::back_inserter(timeOutQue_));
        timers_.erase(timers_.begin(), bound);
    }
}

void poller::dealExpiredTimers()
{
    for (const auto& t: timeOutQue_) {
        t.second->run();
        if (t.second->isRepeat()) {
            t.second->reset();
            timers_.insert(std::pair<Timestamp, Timer*>(t.second->getExpiration(), t.second));
        }
    }
}

void poller::dealPendingFunctors()
{
    for (auto& f : pendingFunctors_) {
        f();
    }
}

void poller::addPendingFunction(PendingCallbackFunc func)
{
    pendingFunctors_.push_back(func);
}

void  Channel::handleEvents()
{
    if (revents_ & kWriteEvent) {
        if (writeCallbackFunc_) {
            writeCallbackFunc_();
        }
    }

    if (revents_ & kReadEvent) {
        if (readCallbackFunc_) {
            readCallbackFunc_();
        }
    }

    if (revents_ & kErrorEvent) {
        if (errorCallbackFunc_) {
            errorCallbackFunc_();
        }
    }

}

void PollPoller::runPoll()
{
    //准备poll需要的监听事件
    preparePollEvents();
    int ret = poll(pollfds_.data(), pollfds_.size(), kIntervalTime);
    if (ret < 0) {
        perror("poll error");
    }

    //提取有事件发生的Channel
    getActiveChannels();
    //提取到期的定时器事件
    getExpiredTimers();
    //处理channel上的事件
    handleEvents();
    //处理定时器事件
    dealExpiredTimers();
    //处理挂起的任务
    dealPendingFunctors();
}

void PollPoller::preparePollEvents()
{
    pollfds_.clear();
    for (const auto& p : pollChannels_) {
        struct pollfd ev;
        ev.fd = p.first;
        ev.events = (short)p.second->getEvents();
        pollfds_.push_back(ev);
    }
}

void PollPoller::getActiveChannels()
{
    activeChannels_.clear();
    for (const auto & ev : pollfds_) {
        if (ev.revents & (POLLOUT | POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL)) {
            Channel * ch = pollChannels_[ev.fd];
            ch->setRevents(ev.revents);
            activeChannels_.push_back(ch);
        }
    }
}

void PollPoller::handleEvents()
{
    for (const auto& c: activeChannels_) {
        c->handleEvents();
    }
}


