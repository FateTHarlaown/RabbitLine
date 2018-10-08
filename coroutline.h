//
// Created by NorSnow_ZJ on 2018/7/5.
//

#ifndef RABBITLINE_COROUTLINE_H
#define RABBITLINE_COROUTLINE_H

#include <ucontext.h>
#include <thread>
#include <functional>
#include <stack>
#include <memory>
#include <unordered_map>
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
    UNKNOWN
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
        state = FREE;
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
    using WorkersMap = std::unordered_map<int64_t, coroutline_t *>;

    Scheduler();
    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;
    ~Scheduler();
    void mainLoop();
    void stopLoop();
    void yield();
    int64_t create(Func func);
    int64_t getRunningWoker();
    void resume(int64_t id);
    State getStatus(int64_t id);

private:
    void saveCoStack(int64_t id);
    void jumpToRunningCo();
    void workerRoutline();
    void initSwitchCtx();

private:
    static const int kMaxCoroutlineNum = 1000000;
    static const uint64_t kMaxStackSize = 1024 * 1024 *8;

    int64_t seq_;
    uint64_t activeWorkerNum_;
    WorkersMap workers_;
    std::stack<int64_t> callPath_;
    char *stack_;
    bool run_;
    int64_t runningWorker_;
    ucontext_t loopCtx_;
    ucontext_t switchCtx_;
    bool switchInited_;
    char *switchStack_;
    Poller *poller_;
};

}

#endif //RABBITLINE_COROUTLINE_H
