//
// Created by NorSnow_ZJ on 2018/7/5.
//

#ifndef COROUTLINE_H
#define COROUTLINE_H

#include <ucontext.h>
#include <thread>
#include <functional>
#include <stack>
#include "poller.h"

namespace RabbitLine
{

class Scheduler;

/*通过这个函数初始化并获取获取本线程的Scheduler*/
extern Scheduler *getLocalScheduler();

using Func = std::function<void()>;
//协程运行状态
enum State
{
    FREE,
    RUNABLE,
    RUNNING,
    SUSPEND,
};

//每个协程的初始栈大小
static const int kDefaultStackSize = 1024 * 128;


typedef struct coroutline
{
    State state;
    ucontext_t ctx;
    char *stack;
    Func func;
    uint64_t stackSize;
    uint64_t stackCapacity;

    coroutline(const coroutline &) = delete;

    coroutline &operator=(const coroutline &) = delete;

    coroutline() : stack(nullptr), stackSize(0), stackCapacity(kDefaultStackSize)
    {
        stack = new char[stackCapacity];
    }

    ~coroutline()
    {
        if (stackCapacity > 0)
        {
            delete[] stack;
        }
    }
} coroutline_t;

class Scheduler
{
public:
    Scheduler();
    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;
    ~Scheduler();
    void mainLoop();
    void stopLoop();
    void yield();
    int create(Func func);
    int getRunningWoker();
    void resume(int id);
    State getStatus(int id);

private:
    void saveCoStack(int id);
    void jumpToRunningCo();
    int getIdleWorker();
    void workerRoutline();
    void initSwitchCtx();

private:
    static const int kMaxCoroutlineNum = 10000;
    static const uint64_t kMaxStackSize = 1024 * 1024;

    coroutline_t *workers_;
    char *stack_;
    std::stack<int> *callPath_;
    bool run_;
    int runningWorker_;
    ucontext_t loopCtx_;
    ucontext_t switchCtx_;
    bool switchInited_;
    char *switchStack_;
    Poller *poller_;

};

}

#endif //COROUTLINE_H
