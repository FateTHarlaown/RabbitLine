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
    FINISHED
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
    char stack[kDefaultStackSize];
    Func func;
    arg_t arg;
} coroutline_t;

class scheduler
{
public:
    using WorkerQueue= std::deque<int>;

    scheduler();
    scheduler(const scheduler &) = delete;
    scheduler &operator=(const scheduler &) = delete;
    ~scheduler();

    void start();

    void stop()
    {
        run_ = false;
        loopThread.join();
    }

    State getCoroutLineState(int id)
    {
        return workers_[id].state;
    }

    void yeild();
    int create(Func func, void * arg);
    void resume(int id);

private:
    void mainLoop();
    int getIdleWorker();
    void workerRoutline();

private:
    static const int kMaxCoroutlineNum = 1000;

    coroutline_t * workers_;
    std::thread loopThread;
    std::mutex mu_;
    std::atomic_bool run_;
    int runningWorker_;
    ucontext_t schedulerCtx_;
};

#endif //COROUTLINE_H
