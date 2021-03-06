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

using ChannelPtr = std::shared_ptr<Channel>;

typedef struct fdinfo {
    int userFlag;
    struct sockaddr_in dest;
    int domain;
    ChannelPtr ch;
    time_t writeTimeout;/*milliseconds*/
    time_t readTimeout;/*milliseconds*/
} fdinfo_t;

using FdInfoPtr = std::shared_ptr<fdinfo_t>;

static std::unordered_map<int, FdInfoPtr> fdMap;
static thread_local bool hookFlag = false;

static const int kWaitReadEventType = 0;
static const int kWaitWriteEventType = 1;

using sys_read_ptr = ssize_t (*)(int fd, void* buf, size_t nbyte);
using sys_write_ptr= ssize_t (*)(int fd, const void* buf, size_t nbyte);
using sys_socket_ptr= int (*)(int domain, int type, int protocol);
using sys_accept_ptr = int (*)(int fd, struct sockaddr *addr, socklen_t *len);
using sys_setsockopt_ptr = int (*)(int fd, int level, int option_name,
                                   const void *option_value, socklen_t option_len);
using sys_connect_ptr = int (*)(int socket, const struct sockaddr *address, socklen_t address_len);
using sys_close_ptr = int (*)(int fd);
using sys_poll_ptr = int(*)(struct pollfd fds[], nfds_t nfds, int timeout);
using sys_fcntl_ptr = int (*)(int feilds, int cmd, ...);
using sys_sleep_ptr = unsigned int (*)(int seconds);

static sys_read_ptr sys_read = (sys_read_ptr)dlsym(RTLD_NEXT, "read");
static sys_write_ptr sys_write = (sys_write_ptr)dlsym(RTLD_NEXT, "write");
static sys_socket_ptr sys_socket = (sys_socket_ptr)dlsym(RTLD_NEXT, "socket");
static sys_accept_ptr sys_accept = (sys_accept_ptr)dlsym(RTLD_NEXT, "accept");
static sys_connect_ptr sys_connect = (sys_connect_ptr)dlsym(RTLD_NEXT, "connect");
static sys_close_ptr sys_close = (sys_close_ptr)dlsym(RTLD_NEXT, "close");
static sys_poll_ptr sys_poll = (sys_poll_ptr)dlsym(RTLD_NEXT, "poll");
static sys_fcntl_ptr  sys_fcntl = (sys_fcntl_ptr)dlsym(RTLD_NEXT, "fcntl");
static sys_setsockopt_ptr  sys_setsockopt = (sys_setsockopt_ptr)dlsym(RTLD_NEXT, "setsockopt");
static sys_sleep_ptr sys_sleep = (sys_sleep_ptr)dlsym(RTLD_NEXT, "sleep");

static FdInfoPtr getFdInfo(int fd);
static FdInfoPtr allocFdInfo(int fd);
static void waitUntilEventOrTimeout(int fd, int waitType);
static void freeFdInfo(int fd);

int RabbitLine::CoSocket(int domain, int type, int protocol)
{
    //std::cout << "call my socket! " << std::endl;
    int fd = sys_socket(domain, type, protocol);
    if (fd < 0) {
        return fd;
    }

    FdInfoPtr info = allocFdInfo(fd);
    if (info) {
        info->domain = domain;
        CoFcntl(fd, F_SETFL, sys_fcntl(fd, F_GETFL, 0));
    }

    return fd;
}

int socket(int domain, int type, int protocol)
{
    //std::cout << "call my socket! " << std::endl;
    if (!isEnableHook()) {
        return sys_socket(domain, type, protocol);
    }

    return CoSocket(domain, type, protocol);
}

int RabbitLine::CoAccept(int fd, struct sockaddr *addr, socklen_t *len)
{
    FdInfoPtr info = getFdInfo(fd);
    if (!info || (O_NONBLOCK & info->userFlag)) {
        std::cout << "no block, call sys accept" << std::endl;
        return sys_accept(fd, addr, len);
    }

    //std::cout << "call my co accept" << std::endl;
    waitUntilEventOrTimeout(fd, kWaitReadEventType);

    int cli = sys_accept(fd, addr, len);
    if (cli < 0) {
        return cli;
    }

    FdInfoPtr cliInfo = allocFdInfo(cli);
    if (cliInfo) {
        info->domain = info->domain;
        CoFcntl(cli, F_SETFL, sys_fcntl(cli, F_GETFL, 0));
    }
    return cli;
}

int accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    if (!isEnableHook()) {
        return sys_accept(fd, addr, len);
    }

    return CoAccept(fd, addr, len);
}

int RabbitLine::CoSetsockopt(int fd, int level, int option_name,
               const void *option_value, socklen_t option_len)
{
    //std::cout << "call my setsocketopt! " << std::endl;
    FdInfoPtr info = getFdInfo(fd);
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

int setsockopt(int fd, int level, int option_name,
               const void *option_value, socklen_t option_len)
{
    if (!isEnableHook()) {
        return sys_setsockopt(fd, level, option_name, option_value, option_len);
    }

    return CoSetsockopt(fd, level, option_name, option_value, option_len);
}

ssize_t RabbitLine::CoRead(int fd, void * buf, size_t nbyte)
{
    //std::cout << "call my coread! " << std::endl;
    FdInfoPtr info = getFdInfo(fd);
    if (!info || (O_NONBLOCK & info->userFlag)) {

        return sys_read(fd, buf, nbyte);
    }

    waitUntilEventOrTimeout(fd, kWaitReadEventType);

    return sys_read(fd, buf, nbyte);
}

ssize_t read(int fd, void * buf, size_t nbyte) {
    //std::cout << "call my read! " << std::endl;
    if (!isEnableHook()) {
        return sys_read(fd, buf, nbyte);
    }

    return CoRead(fd, buf, nbyte);
}

ssize_t RabbitLine::CoWrite(int fd, const void * buf, size_t nbyte)
{
    //std::cout << "call my cowrite! " << std::endl;
    FdInfoPtr info = getFdInfo(fd);
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

ssize_t write(int fd, const void * buf, size_t nbyte)
{
    //std::cout << "call my write! " << std::endl;
    if (!isEnableHook()) {
        return sys_write(fd, buf, nbyte);
    }

    return CoWrite(fd, buf, nbyte);
}

int RabbitLine::CoConnect(int fd, const struct sockaddr *address, socklen_t address_len)
{
    int ret = sys_connect(fd, address, address_len);
    FdInfoPtr info = getFdInfo(fd);
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

    ChannelPtr ch = info->ch;
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

int connect(int fd, const struct sockaddr *address, socklen_t address_len)
{
    if (!isEnableHook()) {
        return sys_connect(fd, address, address_len);
    }

    return CoConnect(fd, address, address_len);
}

int RabbitLine::CoFcntl(int feilds, int cmd, ...)
{
    //std::cout << "call my fcntl" << std::endl;
    if (feilds < 0) {
        return __LINE__;
    }

    va_list args;
    va_start(args, cmd);
    int ret = -1;
    FdInfoPtr info = getFdInfo(feilds);
    switch (cmd) {
        case F_SETFL: {
            int param = va_arg(args, int);
            int flag = param;
            if (info && isEnableHook()) {
                //std::cout << "my fcntl set flag and no block" << std::endl;
                flag |= O_NONBLOCK;
            }

            ret = sys_fcntl(feilds, cmd, flag);
            if (0 == ret && info) {
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

int fcntl(int feilds, int cmd, ...)
{
    //std::cout << "call my fcntl" << std::endl;
    if (feilds < 0) {
        return __LINE__;
    }

    va_list args;
    va_start(args, cmd);
    int ret = -1;
    FdInfoPtr info = getFdInfo(feilds);
    switch (cmd) {
        case F_SETFL: {
            int param = va_arg(args, int);
            int flag = param;
            if (info && isEnableHook()) {
                //std::cout << "my fcntl set flag and no block" << std::endl;
                flag |= O_NONBLOCK;
            }

            ret = sys_fcntl(feilds, cmd, flag);
            if (0 == ret && info) {
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


int RabbitLine::CoClose(int fd)
{
    freeFdInfo(fd);
    return sys_close(fd);
}

int close(int fd)
{

    if (!isEnableHook()) {
        return sys_close(fd);
    }

    return sys_close(fd);
}

unsigned int RabbitLine::CoSleep(unsigned int seconds)
{
    //std::cout << "call my sleep " << std::endl;
    Scheduler * sc = getLocalScheduler();
    Poller * po = getLocalPoller();
    double prev  = Timestamp::nowMicroSeconds();
    int64_t  timerid = po->addTimer(Timestamp::nowAfterSeconds(seconds),
                                    std::bind(&Scheduler::resume, sc, sc->getRunningWoker()));
    sc->yield();
    po->removeTimer(timerid);
    double now = Timestamp::nowMicroSeconds();
    double time = now - prev;
    assert(time > 0);
    return static_cast<unsigned int>(time);
}


unsigned int sleep(unsigned int seconds)
{
    if (!isEnableHook()) {
        return sys_sleep(seconds);
    }

    return CoSleep(seconds);
}

FdInfoPtr getFdInfo(int fd)
{
    if (fdMap.find(fd) != fdMap.end()) {
        return fdMap[fd];
    }

    return nullptr;
}

FdInfoPtr  allocFdInfo(int fd)
{
    if (fdMap.find(fd) == fdMap.end()) {
        FdInfoPtr info = std::make_shared<fdinfo_t>();
        info->readTimeout = 1000;
        info->writeTimeout = 1000;
        info->ch = std::make_shared<Channel>(getLocalPoller(), fd);

        fdMap[fd] = info;
        return info;
    }

    return nullptr;
}

void waitUntilEventOrTimeout(int fd, int waitType)
{
    Scheduler * sc = getLocalScheduler();
    Poller * po = getLocalPoller();
    FdInfoPtr info = getFdInfo(fd);
    /*调用者必须保证这个fd已经分配*/
    assert(info);
    ChannelPtr ch = info->ch;

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
        fdMap.erase(fd);
    }
}

void RabbitLine::enableHook()
{
    hookFlag = true;
}


void RabbitLine::disableHook()
{
    hookFlag = false;
}

bool RabbitLine::isEnableHook()
{
    return hookFlag;
}

