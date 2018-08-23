//
// Created by NorSnow_ZJ on 2018/8/23.
//

#ifndef COROUTLINE_CHANNEL_H
#define COROUTLINE_CHANNEL_H

#include <poll.h>
#include "callbacks.h"
#ifdef __linux__
#include <sys/epoll.h>
#endif

class Poller;

class Channel
{
public:
    Channel(Poller* po, int fd) : poller_(po), fd_(fd), events_(kNoEvent)
    {
    }

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    int getEvents();
    void setRevents(int revents);
    void enableWirte();
    void enableRead();
    void disableWrite();
    void disableRead();
    void clearEvents();
    int getFd();
    void addToPoller();
    void removeFromPoller();
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

#endif //COROUTLINE_CHANNEL_H