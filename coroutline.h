//
// Created by NorSnow_ZJ on 2018/7/5.
//

#ifndef COROUTLINE_H
#define COROUTLINE_H

#include <ucontext.h>
#include <thread>
#include <deque>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>

class scheduler;

//协程运行状态
enum State {
    FREE,
    RUNABLE,
    RUNNING,
    SUSPEND,
};

typedef struct args {
    scheduler * sc;
    void * arg;
} arg_t;

using Func = void (*)(arg_t);
static const int kDefaultStackSize = 1024 * 128;

typedef struct coroutline {
    std::atomic<State> state;
    ucontext_t ctx;
    char * stack;
    Func func;
    arg_t arg;
    uint64_t stackSize;
    uint64_t stackCapacity;

    coroutline() : stack(nullptr), stackSize(0), stackCapacity(kDefaultStackSize)
    {
        stack = new char[stackCapacity];
    }

    ~coroutline()
    {
        if (stackCapacity > 0) {
            delete [] stack;
        }
    }
} coroutline_t;

class scheduler
{
public:
    scheduler();
    scheduler(const scheduler &) = delete;
    scheduler &operator=(const scheduler &) = delete;
    ~scheduler();

    void startLoopInThread();

    void stopLoop()
    {
        run_ = false;
        loopThread.join();
    }

    void yeild();
    int create(Func func, void * arg);
    void resume(int id);
    State getStatus(int id);

private:
    void saveCoStack(int id);
    void mainLoop();
    int getIdleWorker();
    void workerRoutline();

private:
    static const int kMaxCoroutlineNum = 10000;
    static const uint64_t kMaxStackSize = 1024 * 1024;

    coroutline_t * workers_;
    char * stack_;
    std::thread loopThread;
    std::mutex mu_;
    std::atomic_bool run_;
    int runningWorker_;
    ucontext_t schedulerCtx_;
};

#endif //COROUTLINE_H
