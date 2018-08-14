//
// Created by NorSnow_ZJ on 2018/7/5.
//

#include <cstring>
#include <cassert>
#include <iostream>
#include "coroutline.h"

Scheduler::Scheduler() : runningWorker_(-1)
{
    workers_ = new coroutline_t[kMaxCoroutlineNum];
    stack_ = new char[kMaxStackSize];

    for (int i = 0; i < kMaxCoroutlineNum; ++i) {
        workers_->state = FREE;
    }

}

Scheduler::~Scheduler()
{
    delete [] workers_;
    delete [] stack_;
}

void Scheduler::startLoopInThread()
{
    run_ = true;
    loopThread = std::move(std::thread(std::bind(&Scheduler::mainLoop, this)));
}

int Scheduler::getIdleWorker()
{
    for (int i = 0; i < kMaxCoroutlineNum; ++i) {
        if (workers_[i].state == FREE) {
            return i;
        }
    }

    return -1;
}

int Scheduler::create(Func func, void *arg)
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

void Scheduler::resume(int id)
{
    //std::cout << "resume worker " << id << "state: " << workers_[id].state << std::endl;
    switch (workers_[id].state) {
        case RUNABLE:
            getcontext(&workers_[id].ctx);

            workers_[id].ctx.uc_stack.ss_sp = stack_;
            workers_[id].ctx.uc_stack.ss_size = kMaxStackSize;
            workers_[id].ctx.uc_sigmask = {0};
            workers_[id].ctx.uc_link = &schedulerCtx_;
            runningWorker_ = id;
            workers_[id].state = RUNNING;

            makecontext(&workers_[id].ctx, (void (*)(void))(&Scheduler::workerRoutline), 1, this);
            swapcontext(&schedulerCtx_, &workers_[id].ctx);

            break;

        case SUSPEND:
            //std::cout << "worker: " << id << " resume suspend" << std::endl;
            memcpy(stack_+kMaxStackSize - workers_[id].stackSize, workers_[id].stack, workers_[id].stackSize);
            runningWorker_ = id;
            workers_[id].state = RUNNING;

            swapcontext(&schedulerCtx_, &workers_[id].ctx);

            break;

        default:
            break;
    }
}

State Scheduler::getStatus(int id)
{
    return workers_[id].state;
}

void Scheduler::saveCoStack(int id)
{
    char dummy = 0;
    uint64_t size = stack_ + kMaxStackSize - &dummy;
    assert(size <= kMaxStackSize);

    if (size > workers_[id].stackSize) {
        delete [] workers_[id].stack;
        workers_[id].stack = new char[size];
    }

    workers_[id].stackSize = size;
    workers_[id].stackCapacity = size;

    memcpy(workers_[id].stack, &dummy, size);
}

void Scheduler::yeild()
{
    if (runningWorker_ != -1) {
        int id = runningWorker_;
        runningWorker_ = -1;
        workers_[id].state = SUSPEND;
        saveCoStack(id);
        swapcontext(&workers_[id].ctx, &schedulerCtx_);
    }
}

void Scheduler::mainLoop()
{
    while (run_) {

        for (int i = 0; i < kMaxCoroutlineNum; ++i) {
            if (workers_[i].state == RUNABLE || workers_[i].state == SUSPEND) {
                int id = i;
                resume(id);
            }
        }

    }
}

void Scheduler::workerRoutline()
{
    int id = runningWorker_;

    workers_[id].func(workers_[id].arg);
    runningWorker_ = -1;
    workers_[id].state = FREE;
}