//
// Created by NorSnow_ZJ on 2018/7/27.
//

#include "poller.h"

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

void PollPoller::poll()
{
    //准备poll需要的监听时间
    pollfds_.clear();
    for (const auto& p : pollChannels_) {
        struct pollfd ev;
        ev.fd = p.first;
        ev.events = (short)p.second->getEvents();
        pollfds_.push_back(ev);
    }

    int ret = poll(pollfds_.data(), pollfds_.size(), kIntervalTime);
    if (ret < 0) {
        perror("poll error");
    }

    //提取有事件发生的Channel
    activeChannels_.clear();
    for (const auto & ev : pollfds_) {
        if (ev.revents & (POLLOUT | POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL)) {
            Channel * ch = pollChannels_[ev.fd];
            ch->setRevents(ev.revents);
            activeChannels_.push_back(ch);
        }
    }

    //todo:处理定时器超时

    handleEvents();
}