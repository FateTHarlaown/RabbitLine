//
// Created by NorSnow_ZJ on 2018/7/5.
//

#include <cstring>
#include <cassert>
#include <iostream>
#include "coroutline.h"

__thread Scheduler * localScheduler = NULL;

Scheduler * getLocalScheduler()
{
    if (!localScheduler) {
        localScheduler = new Scheduler();
    }

    return localScheduler;
}

Scheduler::Scheduler() : runningWorker_(-1), run_(false), switchInited_(false)
{
    workers_ = new coroutline_t[kMaxCoroutlineNum];
    stack_ = new char[kMaxStackSize];

    for (int i = 0; i < kMaxCoroutlineNum; ++i)
    {
        workers_->state = FREE;
    }

    callPath_ = new std::stack<int>();
    switchStack_ = new char[1024];
    poller_ = getLocalPoller();
}

Scheduler::~Scheduler()
{
    delete [] switchStack_;
    delete callPath_;
    delete [] workers_;
    delete [] stack_;
}

void Scheduler::initSwitchCtx()
{
    getcontext(&switchCtx_);
    switchCtx_.uc_stack.ss_sp = switchStack_;
    switchCtx_.uc_stack.ss_size = 1024;
    switchCtx_.uc_sigmask = {0};
    /*这个上下文在中途就会跳转，绝不会执行完*/
    switchCtx_.uc_link = NULL;
    makecontext(&switchCtx_, reinterpret_cast<void (*)()>(&Scheduler::jumpToRunningCo), 1, this);
    switchInited_ = true;
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
    int id = getIdleWorker();
    if (id == -1) {
        return -1;
    }

    if (!switchInited_) {
        initSwitchCtx();
    }

    workers_[id].func = func;
    workers_[id].arg = arg;
    workers_[id].state = RUNABLE;

    return id;
}

void Scheduler::resume(int id)
{
    //std::cout << "resume worker " << id << "state: " << workers_[id].state << std::endl;
    /*获取当前上下文，如果处于协程调用链的底层，则该resum是mainLoop调用的，上下文保存在loopCtx_中*/
    ucontext_t * curCtx = NULL;
    if (!callPath_->empty()) {
        int curId = callPath_->top();
        /*如果上级调用者不是mainLoop，则需要保存栈信息*/
        saveCoStack(curId);
        assert(runningWorker_ == curId);
        curCtx = &workers_[curId].ctx;
    } else {
        curCtx = &loopCtx_;
    }

    callPath_->push(id);
    switch (workers_[id].state) {
        case RUNABLE:
            getcontext(&workers_[id].ctx);

            workers_[id].ctx.uc_stack.ss_sp = stack_;
            workers_[id].ctx.uc_stack.ss_size = kMaxStackSize;
            workers_[id].ctx.uc_sigmask = {0};
            runningWorker_ = id;
            workers_[id].state = RUNNING;

            makecontext(&workers_[id].ctx, reinterpret_cast<void (*)()>(&Scheduler::workerRoutline), 1, this);
            swapcontext(curCtx, &workers_[id].ctx);

            break;

        case SUSPEND:
            /*
             * 处于SUSPEND状态的协程需要拷贝栈信息，
             * 通过运行在独立栈中的jumpToCo来拷贝,当上级调用者是mainLoop时其实可以不用
             * */
            runningWorker_ = id;
            workers_[id].state = RUNNING;
            swapcontext(curCtx, &switchCtx_);

            break;

        default:
            assert(0);
            break;
    }
}

State Scheduler::getStatus(int id)
{
    return workers_[id].state;
}

int Scheduler::getRunningWoker()
{
    return runningWorker_;
}

void Scheduler::saveCoStack(int id)
{
    char dummy = 0;
    /*
     * 计算当协程已经使用的程栈大小
     * 由于栈由高地址向低地址增长,stack_ + kMaxStackSize表示栈底
     * dummy的地址代表栈顶
     * */
    uint64_t size = stack_ + kMaxStackSize - &dummy;
    assert(size <= kMaxStackSize);

    /*自动增长栈大小*/
    if (size > workers_[id].stackSize) {
        delete [] workers_[id].stack;
        workers_[id].stack = new char[size];
    }

    workers_[id].stackSize = size;
    workers_[id].stackCapacity = size;

    memcpy(workers_[id].stack, &dummy, size);
}

/*
 * 由于使用共享栈，非对称协程之间的调用要拷贝栈，
 * 必须用一个运行于单独的栈空间的特殊函数来拷贝
 * */
void Scheduler::jumpToRunningCo()
{
    assert(runningWorker_ >= 0);
    memcpy(stack_+kMaxStackSize - workers_[runningWorker_].stackSize, workers_[runningWorker_].stack, workers_[runningWorker_].stackSize);
    setcontext(&workers_[runningWorker_].ctx);
}

void Scheduler::yield()
{
    if (runningWorker_ != -1) {

        int curId = callPath_->top();
        assert(runningWorker_ == curId);
        callPath_->pop();
        int nextId;
        if (!callPath_->empty()) {
            nextId = callPath_->top();
            runningWorker_ = nextId;
        } else {
            runningWorker_ = -1;
        }

        workers_[curId].state = SUSPEND;
        saveCoStack(curId);
        if (runningWorker_ == -1) {
            /*跳转到mainLoop不需要进行栈拷贝*/
            swapcontext(&workers_[curId].ctx, &loopCtx_);
        } else {
            swapcontext(&workers_[curId].ctx, &switchCtx_);
        }
    }
}

void Scheduler::mainLoop()
{
    run_ = true;
    while (run_) {
        poller_->runPoll();
    }
}

void Scheduler::stopLoop()
{
    run_ = false;
}

void Scheduler::workerRoutline()
{
    int id = runningWorker_;
    workers_[id].func(workers_[id].arg);
    workers_[id].state = FREE;
    assert(runningWorker_ == callPath_->top());
    callPath_->pop();
    /*若上级调用者也是一个普通协程而不是mainLoop则要拷贝栈*/
    if (!callPath_->empty()) {
        runningWorker_ = callPath_->top();
        setcontext(&switchCtx_);
    } else {
        runningWorker_ = -1;
        setcontext(&loopCtx_);
    }
}