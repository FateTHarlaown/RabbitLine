//
// Created by NorSnow_ZJ on 2018/7/5.
//

#include "coroutline.h"

scheduler::scheduler() : runningWorker_(-1)
{
    workers_ = new coroutline_t[kMaxCoroutlineNum];
    for (int i = 0; i < kMaxCoroutlineNum; ++i) {
        workers_->state = FREE;
    }
}

scheduler::~scheduler()
{
    delete [] workers_;
}

void scheduler::start()
{
    run_ = true;
    loopThread = std::move(std::thread(std::bind(mainLoop, this)));
}

int scheduler::getIdleWorker()
{
    for (int i = 0; i < kMaxCoroutlineNum; ++i) {
        if (workers_[i].state == FREE) {
            return i;
        }
    }

    return -1;
}

int scheduler::create(Func func, void *arg)
{
    //同一时间只能有一个任务处于创建中
    std::lock_guard<std::mutex> guard(mu_);
    int id = getIdleWorker();
    if (id == -1) {
        return -1;
    }

    arg_t routLineArg;
    routLineArg.arg = arg;
    routLineArg.sc = this;

    workers_[id].func = func;
    workers_[id].arg = routLineArg;
    workers_[id].state = RUNABLE;

    return id;
}

void scheduler::resume(int id)
{
    switch (workers_[id].state) {
        case RUNABLE:
            getcontext(&workers_[id].ctx);

            workers_[id].ctx.uc_stack.ss_sp = workers_[id].stack;
            workers_[id].ctx.uc_stack.ss_size = kDefaultStackSize;
            workers_[id].ctx.uc_sigmask = 0;
            workers_[id].ctx.uc_link = &schedulerCtx_;

            makecontext(&workers_[id].ctx, (void (*)(void))(&scheduler::workerRoutline), 1, this);

        case SUSPEND:

            runningWorker_ = id;
            workers_[id].state = RUNNING;

            swapcontext(&schedulerCtx_, &workers_[id].ctx);

            break;

        default:
            break;
    }
}

void scheduler::yeild()
{
    if (runningWorker_ != -1) {
        int id = runningWorker_;
        runningWorker_ = -1;
        workers_[id].state = SUSPEND;

        swapcontext(&workers_[id].ctx, &schedulerCtx_);
    }
}

void scheduler::mainLoop()
{
    while (run_) {
        int id;

        for (int i = 0; i < kMaxCoroutlineNum; ++i) {
            if (workers_[i].state == RUNABLE || workers_[i].state == SUSPEND) {
                id = i;
                resume(id);
            }
        }

    }
}

void scheduler::workerRoutline()
{
    int id = runningWorker_;

    workers_[id].func(workers_[id].arg);
    runningWorker_ = -1;
    workers_[id].state = FREE;
}