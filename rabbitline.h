//
// Created by NorSnow_ZJ on 2018/9/25.
//

#ifndef RABBITLINE_H
#define RABBITLINE_H

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
 * 启用hook系统调用的功能（= = 其实目前还没有关闭功能）
 */
int enableHook();

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

}

#endif //RABBITLINE_H
