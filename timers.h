//
// Created by NorSnow_ZJ on 2018/8/1.
//

#ifndef COROUTLINE_TIMERS_H
#define COROUTLINE_TIMERS_H

#include <sys/types.h>
#include "callbacks.h"

class Timestamp
{
public:
    Timestamp(double microSeconds = 0.0) : microSecondsSinceEpoch_(microSeconds)
    {
    }

    ~Timestamp()
    {}

    bool valid();
    int64_t microSecondsSinceEpoch();
    std::string toString() const;

    static Timestamp now();
    static Timestamp nowAfter(double seconds);
    static double nowMicroSeconds();
    static const int kMicroSecondsPerSecond = 1000 * 1000;
    friend bool operator <(Timestamp l, Timestamp r);
    friend bool operator ==(Timestamp l, Timestamp r);
private:
    int64_t microSecondsSinceEpoch_;
};

bool operator <(Timestamp l, Timestamp r);
bool operator ==(Timestamp l, Timestamp r);

class Timer
{
public:
    Timer(Timestamp expirationTime, TimeoutCallbackFunc callback, bool repeat = false, int interval = 0) :
            expirationTime_(expirationTime), callback_(callback), repeat_(repeat), interval_(interval)
    {

    }
    Timer(const Timer&) = delete;
    Timer&operator=(const Timer&) = delete;
    void reset();
    void run();
    bool isRepeat();
    Timestamp getExpiration();
private:
    Timestamp expirationTime_;
    TimeoutCallbackFunc callback_;
    bool repeat_;
    const int interval_;
};

#endif //COROUTLINE_TIMERS_H

