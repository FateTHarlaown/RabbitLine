//
// Created by NorSnow_ZJ on 2018/9/25.
//

#ifndef RABBITLINE_H
#define RABBITLINE_H

#include <sys/socket.h>
#include "coroutline.h"

namespace RabbitLine {

/*
 *  创建一个协程.
 *  @param fn: std::function<void()>类型的回调函数.
 *  @return 创建成功，返回协程的id；否则返回-1.
 */
int64_t create(Func fn);

/*
 *  调度运行一个协程.
 *  @param id: 待运行协程的id.
 *  @return void.
 */
void resume(int64_t id);

/*
 * 获取本协程id
 * @return int64_t
 */
int64_t getMyCoId();

/*
 * 让出cpu本协程cpu使用权
 */
void yield();

/*
 * 开始主协程的事件循环.
 */
void eventLoop();

/*
 * 停止主协程的事件循环
 */
void stopLoop();

/*
 * 启用hook系统调用的功能.
 */
void enableHook();

/*
 * 关闭hook系统调用的功能.
 */
void disableHook();

/*
 * 是否开启著hook系统调用.
 * @return: bool值，true表示开启，false未开启.
 */
bool isEnableHook();

/*
 * 添加一个定时器
 * @param expirationTime: 定时器到期时间
 * @param callback: std::function<void()>类型的回调函数.
 * @param repeat: 是否重复触发，默认值为false.
 * @param repeat: 若要重复触发，这个参数表示重复触发的时间间隔，否则无意义.
 * @return: 定时器id
 */
int64_t addTimer(Timestamp expirationTime, TimeoutCallbackFunc callback, bool repeat = false, int interval = 0);

/*
 * 删除一个定时器
 * @param: 定时器id
 */
void removeTimer(int64_t seq);

/*
 * hook的系统调用的内部实现版本
 * 声明在这里用于当关闭hook系统调用功能时在RabbitLine命名空间显式调用
 */
int CoSocket(int domain, int type, int protocol);
int CoAccept(int fd, struct sockaddr *addr, socklen_t *len);
int CoSetsockopt(int fd, int level, int option_name,
               const void *option_value, socklen_t option_len);
int CoFcntl(int feilds, int cmd, ...);
ssize_t CoRead(int fd, void * buf, size_t nbyte);
ssize_t CoWrite(int fd, const void * buf, size_t nbyte);
int CoConnect(int fd, const struct sockaddr *address, socklen_t address_len);
int CoClose(int fd);
unsigned int CoSleep(unsigned int seconds);

}

#endif //RABBITLINE_H
