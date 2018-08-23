//
// Created by NorSnow_ZJ on 2018/7/27.
//

#include <cassert>
#include "poller.h"
#include "channel.h"


int64_t Poller::addTimer(Timestamp expirationTime, TimeoutCallbackFunc callback, bool repeat, int interval)
{
    if (expirationTime < Timestamp::now()) {
        return -1;
    }

    Timer * timer = new Timer(expirationTime, callback, repeat, interval);
    int64_t timerSeq = seq_++;
    waitingTimers_.insert(std::pair<int64_t , Timer*>(timerSeq, timer));
    timers_.insert(std::pair<Timestamp, Timer*>(expirationTime, timer));

    return timerSeq;
}

void Poller::removeTimer(int64_t seq)
{
    auto it = waitingTimers_.find(seq);

    if (it == waitingTimers_.end()) {
        return;
    }

    Timer * delTimer = it->second;
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

    delete delTimer;
}

Poller::~Poller()
{
    for (const auto& p : timers_) {
        if (p.second) {
            delete p.second;
        }
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
        t.second->run();
        if (t.second->isRepeat()) {
            t.second->reset();
            timers_.insert(std::pair<Timestamp, Timer*>(t.second->getExpiration(), t.second));
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

void PollPoller::addChannel(Channel *ch)
{
    if (pollChannels_.end() != pollChannels_.find(ch->getFd()) ) {
        pollChannels_.insert(std::pair<int,Channel*>(ch->getFd(), ch));
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

void PollPoller::handleEvents()
{
    for (const auto& c: activeChannels_) {
        c->handleEvents();
    }
}


