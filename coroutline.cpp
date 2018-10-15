//
// Created by NorSnow_ZJ on 2018/7/5.
//

#include <cstring>
#include <cassert>
#include <iostream>
#include "coroutline.h"

using namespace RabbitLine;

__thread Scheduler * localScheduler = NULL;

Scheduler * RabbitLine::getLocalScheduler()
{
    if (!localScheduler) {
        localScheduler = new Scheduler();
    }

    return localScheduler;
}

Scheduler::Scheduler() : runningWorker_(-1), run_(false),
                         switchInited_(false), seq_(0), activeWorkerNum_(0)
{
    stack_ = new char[kMaxStackSize];
    switchStack_ = new char[1024];
    poller_ = getLocalPoller();
}

Scheduler::~Scheduler()
{
    for (auto & w : workers_) {
        delete w.second;
    }

    delete [] switchStack_;
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

int64_t Scheduler::create(Func func)
{
    if (activeWorkerNum_ >= kMaxCoroutlineNum) {
        return -1;
    }

    int64_t id = seq_++;
    activeWorkerNum_++;

    if (!switchInited_) {
        initSwitchCtx();
    }
    coroutline_t * co = new coroutline_t();
    co->func = func;
    co->state = RUNABLE;
    workers_.insert(WorkersMap::value_type(id, co));

    return id;
}

void Scheduler::resume(int64_t id)
{
    //std::cout << "resume worker " << id << "state: " << workers_[id].state << std::endl;
    if (workers_.end() == workers_.find(id)) {
        return;
    }

    coroutline_t * co = workers_[id];
    /*获取当前上下文，如果处于协程调用链的底层，则该resum是mainLoop调用的，上下文保存在loopCtx_中*/
    ucontext_t * curCtx = NULL;
    if (!callPath_.empty()) {
        int64_t curId = callPath_.top();
        /*如果上级调用者不是mainLoop，则需要保存栈信息*/
        assert(runningWorker_ == curId);
        saveCoStack(curId);
        curCtx = &workers_[curId]->ctx;
    } else {
        curCtx = &loopCtx_;
    }

    callPath_.push(id);
    switch (co->state) {
        case RUNABLE:
            getcontext(&co->ctx);
            co->ctx.uc_stack.ss_sp = stack_;
            co->ctx.uc_stack.ss_size = kMaxStackSize;
            co->ctx.uc_sigmask = {0};
            runningWorker_ = id;
            co->state = RUNNING;
            makecontext(&co->ctx, reinterpret_cast<void (*)()>(&Scheduler::workerRoutline), 1, this);
            swapcontext(curCtx, &co->ctx);

            break;

        case SUSPEND:
            /*
             * 处于SUSPEND状态的协程需要拷贝栈信息，
             * 通过运行在独立栈中的jumpToCo来拷贝,当上级调用者是mainLoop时其实可以不用
             * */
            co->state = RUNNING;
            swapcontext(curCtx, &switchCtx_);

            break;

        default:
            assert(0);
            break;
    }
}

State Scheduler::getStatus(int64_t id)
{
    if (workers_.end() == workers_.find(id)) {
        return UNKNOWN;
    } else {
        return workers_[id]->state;
    }
}

int64_t Scheduler::getRunningWoker()
{
    return runningWorker_;
}

void Scheduler::saveCoStack(int64_t id)
{
    coroutline_t * co = workers_[id];
    char dummy = 0;
    /*
     * 计算当协程已经使用的程栈大小
     * 由于栈由高地址向低地址增长,stack_ + kMaxStackSize表示栈底
     * dummy的地址代表栈顶
     * */
    uint64_t size = stack_ + kMaxStackSize - &dummy;
    assert(size <= kMaxStackSize);

    /*自动增长栈大小*/
    if (size > co->stackSize) {
        delete [] co->stack;
        co->stack = new char[size];
    }

    co->stackSize = size;
    co->stackCapacity = size;

    memcpy(co->stack, &dummy, size);
}

/*
 * 由于使用共享栈，非对称协程之间的调用要拷贝栈，
 * 必须用一个运行于单独的栈空间的特殊函数来拷贝
 * */
void Scheduler::jumpToRunningCo()
{
    if (runningWorker_ != -1) {
        coroutline_t * curCo = workers_[runningWorker_];
        if (curCo->state == FREE) {
            workers_.erase(runningWorker_);
            delete curCo;
        }
    }

    /*除返回主循环mainLoop之外都要拷贝栈*/
    if (!callPath_.empty()) {
        runningWorker_ = callPath_.top();
        coroutline_t * co = workers_[runningWorker_];
        memcpy(stack_+kMaxStackSize - co->stackSize, co->stack, co->stackSize);
        setcontext(&co->ctx);
    } else {
        runningWorker_ = -1;
        setcontext(&loopCtx_);
    }
}

void Scheduler::yield()
{
    if (runningWorker_ == -1) {
        return;
    }

    coroutline_t * co = workers_[runningWorker_];
    assert(runningWorker_ == callPath_.top());
    callPath_.pop();
    co->state = SUSPEND;
    saveCoStack(runningWorker_);
    swapcontext(&co->ctx, &switchCtx_);
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
    coroutline_t * co = workers_[runningWorker_];
    co->func();
    co->state = FREE;
    assert(runningWorker_ == callPath_.top());
    activeWorkerNum_--;
    callPath_.pop();
    //std::cout << runningWorker_ << "has ended!" << std::endl;
    setcontext(&switchCtx_);
}