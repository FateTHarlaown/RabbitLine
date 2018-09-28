//
// Created by NorSnow_ZJ on 2018/8/9.
//

#include <stdarg.h>
#include <dlfcn.h>
#include <unistd.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <unordered_map>
#include <iostream>
#include "coroutline.h"
#include "channel.h"
#include "rabbitline.h"

using namespace RabbitLine;

typedef struct fdinfo {
    int userFlag;
    struct sockaddr_in dest;
    int domain;
    Channel * ch;
    time_t writeTimeout;/*milliseconds*/
    time_t readTimeout;/*milliseconds*/
} fdinfo_t;

static std::unordered_map<int, fdinfo_t*> fdMap;

static const int kWaitReadEventType = 0;
static const int kWaitWriteEventType = 1;

using sys_read_ptr = ssize_t (*)(int fd, void* buf, size_t nbyte);
using sys_write_ptr= ssize_t (*)(int fd, const void* buf, size_t nbyte);
using sys_socket_ptr= int (*)(int domain, int type, int protocol);
using sys_setsockopt_ptr = int (*)(int fd, int level, int option_name,
                                   const void *option_value, socklen_t option_len);
using sys_connect_ptr = int (*)(int socket, const struct sockaddr *address, socklen_t address_len);
using sys_close_ptr = int (*)(int fd);
using sys_poll_ptr = int(*)(struct pollfd fds[], nfds_t nfds, int timeout);
using sys_fcntl_ptr = int (*)(int feilds, int cmd, ...);

static sys_read_ptr sys_read = (sys_read_ptr)dlsym(RTLD_NEXT, "read");
static sys_write_ptr sys_write = (sys_write_ptr)dlsym(RTLD_NEXT, "write");
static sys_socket_ptr sys_socket = (sys_socket_ptr)dlsym(RTLD_NEXT, "socket");
static sys_connect_ptr sys_connect = (sys_connect_ptr)dlsym(RTLD_NEXT, "connect");
static sys_close_ptr sys_close = (sys_close_ptr)dlsym(RTLD_NEXT, "close");
static sys_poll_ptr sys_poll = (sys_poll_ptr)dlsym(RTLD_NEXT, "poll");
static sys_fcntl_ptr  sys_fcntl = (sys_fcntl_ptr)dlsym(RTLD_NEXT, "fcntl");
static sys_setsockopt_ptr  sys_setsockopt = (sys_setsockopt_ptr)dlsym(RTLD_NEXT, "setsockopt");

static fdinfo_t * getFdInfo(int fd);
static fdinfo_t * allocFdInfo(int fd);
static void waitUntilEventOrTimeout(int fd, int waitType);
static void freeFdInfo(int fd);

int socket(int domain, int type, int protocol)
{
    //std::cout << "call my socket! " << std::endl;
    int fd = sys_socket(domain, type, protocol);
    if (fd < 0) {
        return fd;
    }

    fdinfo_t * info = allocFdInfo(fd);
    if (info) {
        info->domain = domain;
        fcntl(fd, F_SETFL, sys_fcntl(fd, F_GETFL, 0));
    }

    return fd;
}

int co_accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    fdinfo * info = getFdInfo(fd);
    if (!info || (O_NONBLOCK & info->userFlag)) {
            return accept(fd, addr, len);
    }


    waitUntilEventOrTimeout(fd, kWaitReadEventType);

    int cli = accept(fd, addr, len);
    if (cli < 0) {
        return cli;
    }

    allocFdInfo(cli);

    return cli;
}

int setsockopt(int fd, int level, int option_name,
               const void *option_value, socklen_t option_len)
{
    //std::cout << "call my setsocketopt! " << std::endl;
    fdinfo * info = getFdInfo(fd);
    if (info && level == SOL_SOCKET) {
        struct timeval * val = (struct timeval *)(option_value);
        if (option_name == SO_RCVTIMEO) {
            info->readTimeout = val->tv_sec * 1000 + val->tv_usec/1000;
        } else if (option_name == SO_SNDTIMEO) {
            info->writeTimeout = val->tv_sec * 1000 + val->tv_usec/1000;
        }
    }

    return sys_setsockopt(fd, level, option_name, option_value, option_len);
}

ssize_t read(int fd, void * buf, size_t nbyte)
{
    //std::cout << "call my read! " << std::endl;
    fdinfo * info = getFdInfo(fd);
    if (!info || (O_NONBLOCK & info->userFlag)) {
        return sys_read(fd, buf, nbyte);
    }

    waitUntilEventOrTimeout(fd, kWaitReadEventType);

    return sys_read(fd, buf, nbyte);
}

ssize_t write(int fd, const void * buf, size_t nbyte)
{
    //std::cout << "call my write! " << std::endl;
    fdinfo * info = getFdInfo(fd);
    if (!info || (O_NONBLOCK & info->userFlag)) {
        return sys_write(fd, buf, nbyte);
    }


    ssize_t ret = sys_write(fd, buf, nbyte);
    if (ret == 0) {
        return ret;
    }

    ssize_t writed = ret > 0 ? ret : 0;
    /*一直到发送完成所有数据或者出现错误为止*/
    while (writed < nbyte) {
        waitUntilEventOrTimeout(fd, kWaitWriteEventType);
        ret = sys_write(fd, buf+writed, nbyte-writed);
        if (ret <= 0) {
            break;
        }

        writed += ret;
    }

    if (ret <=0 && writed == 0) {
        return ret;
    }

    return writed;
}

int connect(int fd, const struct sockaddr *address, socklen_t address_len)
{
    int ret = sys_connect(fd, address, address_len);
    fdinfo * info = getFdInfo(fd);
    if (!info) {
        return ret;
    }

    if (sizeof(info->dest) > address_len) {
        memcpy(&info->dest, address, address_len);
    }

    if (O_NONBLOCK & info->userFlag) {
        return ret;
    }

    /*发生了错误，返回错误码*/
    if (!(ret<0 && errno == EINPROGRESS)) {
        return ret;
    }

    Channel * ch = info->ch;
    /*等待连接成功，最多尝试等待3次，每次25秒超时*/
    for (int i = 0; i < 3; ++i) {
        waitUntilEventOrTimeout(fd, kWaitReadEventType);
        /*连接成功*/
        if (ch->getRevents() & POLLOUT) {
            errno = 0;
            return 0;
        }
        /*发生错误*/
        if (ch->getRevents() != Channel::kNoEvent) {
            break;
        }
    }

    int err;
    socklen_t errlen = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    if (err) {
        errno = err;
    } else {
        errno = ETIMEDOUT;
    }

    return ret;
}

int fcntl(int feilds, int cmd, ...)
{
    //std::cout << "call my fcntl" << std::endl;
    if (feilds < 0) {
        return __LINE__;
    }

    va_list args;
    va_start(args, cmd);
    int ret = -1;
    fdinfo_t * info = getFdInfo(feilds);
    switch (cmd) {
        case F_SETFL: {
            int param = va_arg(args, int);
            int flag = param;
            if (info) {
                flag |= O_NONBLOCK;
            }

            ret = sys_fcntl(feilds, cmd, flag);
            if (ret != 0 && info) {
                info->userFlag = param;
            }

            break;
        }

        case F_GETFL: {
            ret = sys_fcntl(feilds, cmd);
            break;
        }

        case F_GETOWN: {
            ret = sys_fcntl(feilds, cmd);
            break;
        }

        case F_SETOWN: {
            int param = va_arg(args, int);
            ret = sys_fcntl(feilds, cmd, param);
            break;
        }

        case F_SETFD: {
            int param = va_arg(args, int);
            ret = sys_fcntl(feilds, cmd, param);
            break;
        }

        case F_DUPFD: {
            int param = va_arg(args, int);
            ret = sys_fcntl(feilds, cmd, param);
            break;
        }

        case F_GETLK: {
            struct flock * param = va_arg(args, struct flock*);
            ret = sys_fcntl(feilds, cmd, param);
            break;
        }

        case F_SETLK: {
            struct flock * param = va_arg(args, struct flock*);
            ret = sys_fcntl(feilds, cmd, param);
            break;
        }

        case F_SETLKW: {
            struct flock * param = va_arg(args, struct flock*);
            ret = sys_fcntl(feilds, cmd, param);
            break;
        }

        default: {
            break;
        }
    }
}

int close(int fd)
{
    freeFdInfo(fd);
    return sys_close(fd);
}

fdinfo_t * getFdInfo(int fd)
{
    if (fdMap.find(fd) != fdMap.end()) {
        return fdMap[fd];
    }

    return NULL;
}

fdinfo_t * allocFdInfo(int fd)
{
    if (fdMap.find(fd) == fdMap.end()) {
        fdinfo_t * info = new fdinfo_t();
        info->readTimeout = 1000;
        info->writeTimeout = 1000;
        info->ch = new Channel(getLocalPoller(), fd);

        fdMap[fd] = info;
        return info;
    }

    return NULL;
}

void waitUntilEventOrTimeout(int fd, int waitType)
{
    Scheduler * sc = getLocalScheduler();
    Poller * po = getLocalPoller();
    fdinfo * info = getFdInfo(fd);
    /*调用者必须保证这个fd已经分配*/
    assert(info);
    Channel * ch = info->ch;

    ch->clearEvents();
    ch->clearCallbacks();
    if (waitType == kWaitReadEventType) {
        ch->enableRead();
        ch->setReadCallbackFunc(std::bind(&Scheduler::resume, sc, sc->getRunningWoker()));
    } else if (waitType == kWaitWriteEventType) {
        ch->enableWirte();
        ch->setWriteCallbackFunc(std::bind(&Scheduler::resume, sc, sc->getRunningWoker()));
    } else {
        assert(0);
    }

    ch->setErrorCallbackFunc(std::bind(&Scheduler::resume, sc, sc->getRunningWoker()));
    ch->addToPoller();
    int64_t  timerid = po->addTimer(Timestamp::nowAfterMilliSeconds(info->readTimeout),
                                    std::bind(&Scheduler::resume, sc, sc->getRunningWoker()));
    sc->yield();
    /*这个函数结尾不应该调用ch->clearEvents()，
     *因为被唤醒的协程可能还想要知道发生了什么事件*/
    ch->clearCallbacks();
    ch->removeFromPoller();
    po->removeTimer(timerid);
}

void freeFdInfo(int fd)
{
    auto it = fdMap.find(fd);
    if (it != fdMap.end()) {
        delete it->second->ch;
        delete it->second;
        fdMap.erase(fd);
    }
}

int RabbitLine::enableHook()
{
    return 1;
}



