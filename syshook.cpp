//
// Created by NorSnow_ZJ on 2018/8/9.
//
#include <dlfcn.h>
#include <netinet/in.h>
#include <poll.h>
#include <unordered_map>
#include "poller.h"

typedef struct rpchook {
    int userFlag;
    struct sockaddr_in dest;
    int domain;

    struct timeval writeTimeout;
    struct timeval readTimeout;
} rpchook_t;

static std::unordered_map<int, rpchook_t> fdMap;

using sys_read_ptr = ssize_t (*)(int fd, const void* buf, size_t nbyte);
using sys_write_ptr= ssize_t (*)(int fd, const void* buf, size_t nbyte);
using sys_socket_ptr= int (*)(int domain, int type, int protocol);
using sys_connect_ptr = int (*)(int socket, const struct sockaddr *address, socklen_t address_len);
using sys_close_ptr = int (*)(int fd);
using sys_poll_ptr = int(*)(struct pollfd fds[], nfds_t nfds, int timeout);
using sys_fcntl_ptr = int (*)(int fildes, int cmd, ...);

static sys_read_ptr sys_read = (sys_read_ptr)dlsym(RTLD_NEXT, "read");
static sys_write_ptr sys_write = (sys_write_ptr)dlsym(RTLD_NEXT, "write");
static sys_socket_ptr sys_socket = (sys_socket_ptr)dlsym(RTLD_NEXT, "socket");
static sys_connect_ptr sys_connect = (sys_connect_ptr)dlsym(RTLD_NEXT, "connect");
static sys_close_ptr sys_close = (sys_close_ptr)dlsym(RTLD_NEXT, "close");
static sys_poll_ptr sys_poll = (sys_poll_ptr)dlsym(RTLD_NEXT, "poll");
static sys_fcntl_ptr  sys_fcntl = (sys_fcntl_ptr)dlsym(RTLD_NEXT, "fcntl");


int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{

}

int enableHook()
{
    return 1;
}



