//
// Created by NorSnow_ZJ on 2018/8/1.
//

#ifndef COROUTLINE_TIMERS_H
#define COROUTLINE_TIMERS_H

#include <sys/types.h>

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

#endif //COROUTLINE_TIMERS_H

