//
// Created by NorSnow_ZJ on 2018/8/23.
//

#include <iostream>
#include "channel.h"
#include "poller.h"

Channel::Channel(Poller* po, int fd) : poller_(po), isAddedToPoller_(false), fd_(fd), events_(kNoEvent)
{
}

int Channel::getEvents()
{
    return events_;
}

int Channel::getRevents()
{
    return revents_;
}

bool Channel::isAddedToPoller()
{
    return isAddedToPoller_;
}

void Channel::setRevents(int revents)
{
    revents_ = revents;
}

void Channel::enableWirte()
{
    events_ |= kWriteEvent;
    updateToChannel();
}

void Channel::enableRead()
{
    events_ |= kReadEvent;
    updateToChannel();
}

void Channel::disableWrite()
{
    events_ &= ~kWriteEvent;
    updateToChannel();
}

void Channel::disableRead()
{
    events_ &= ~kReadEvent;
    updateToChannel();
}

void Channel::clearEvents()
{
    events_ = kNoEvent;
}

void Channel::clearCallbacks()
{
    readCallbackFunc_ = nullptr;
    writeCallbackFunc_ = nullptr;
    errorCallbackFunc_ = nullptr;
    updateToChannel();
}

void Channel::setReadCallbackFunc(EventCallbackFunc func)
{
    readCallbackFunc_ = func;
    updateToChannel();
}

void Channel::setWriteCallbackFunc(EventCallbackFunc func)
{
    writeCallbackFunc_ = func;
    updateToChannel();
}

void Channel::setErrorCallbackFunc(EventCallbackFunc func)
{
    errorCallbackFunc_ = func;
    updateToChannel();
}

int Channel::getFd()
{
    return fd_;
}

void Channel::addToPoller()
{
    if (!isAddedToPoller_) {
        poller_->addChannel(this);
        isAddedToPoller_ = true;
    }
}

void Channel::removeFromPoller()
{
    if (isAddedToPoller_) {
        poller_->removeChannel(this);
        isAddedToPoller_ = false;
    }
}

void Channel::updateToChannel()
{
    if (isAddedToPoller_) {
        poller_->updateChannel(this);
    }
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
