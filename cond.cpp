//
// Created by NorSnow_ZJ on 2018/10/8.
//

#include <sys/types.h>
#include "cond.h"

using namespace RabbitLine;

void cond::wait()
{
    Scheduler * sc = getLocalScheduler();
    waitList_.push_back(sc->getRunningWoker());
    sc->yield();
}

void cond::timewait(uint ms)
{
    Scheduler * sc = getLocalScheduler();
    Poller * po = getLocalPoller();
    waitList_.push_back(sc->getRunningWoker());
    int64_t timerid = po->addTimer(Timestamp::nowAfterMilliSeconds(ms),
                 std::bind(&Scheduler::resume, sc, sc->getRunningWoker()));
    sc->yield();
    po->removeTimer(timerid);
}

void cond::signal()
{
    if (waitList_.empty()) {
        return;
    }

    Scheduler * sc = getLocalScheduler();
    Poller * po = getLocalPoller();
    int64_t id = waitList_.front();
    waitList_.pop_front();
    po->addPendingFunction(std::bind(&Scheduler::resume, sc, id));
}

void cond::broadcast()
{
    Scheduler * sc = getLocalScheduler();
    Poller * po = getLocalPoller();
    for (auto id : waitList_) {
        po->addPendingFunction(std::bind(&Scheduler::resume, sc, id));
    }

    waitList_.clear();
}
