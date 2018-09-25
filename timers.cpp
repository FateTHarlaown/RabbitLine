//
// Created by NorSnow_ZJ on 2018/8/1.
//
#include <inttypes.h>
#include <sys/time.h>
#include <string>
#include "timers.h"

using namespace RabbitLine;

bool Timestamp::valid()
{
    return microSecondsSinceEpoch_ > 0;
}

int64_t Timestamp::microSecondsSinceEpoch()
{
    return microSecondsSinceEpoch_;
}

std::string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf)-1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}

Timestamp Timestamp::now()
{
    return Timestamp(Timestamp::nowMicroSeconds());
}

Timestamp Timestamp::nowAfterSeconds(size_t seconds)
{
    return Timestamp(Timestamp::nowMicroSeconds() + kMicroSecondsPerSecond * seconds);
}

Timestamp Timestamp::nowAfterMilliSeconds(size_t Milliseconds)
{
    return Timestamp(Timestamp::nowMicroSeconds() + kMicroSecondsPerMilliSecond * Milliseconds);
}

double Timestamp::nowMicroSeconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return seconds * kMicroSecondsPerSecond + tv.tv_usec;
}

bool RabbitLine::operator<(Timestamp l, Timestamp r)
{
    return l.microSecondsSinceEpoch() < r.microSecondsSinceEpoch();
}

bool RabbitLine::operator==(Timestamp l, Timestamp r)
{
    return l.microSecondsSinceEpoch() == r.microSecondsSinceEpoch();
}

void Timer::reset()
{
    if (repeat_) {
        expirationTime_ = Timestamp::nowAfterMilliSeconds(interval_);
    }
}

void Timer::run()
{
    callback_();
}

bool Timer::isRepeat()
{
    return repeat_;
}

int64_t Timer::getTimerid()
{
    return timerid_;
}

Timestamp Timer::getExpiration()
{
    return expirationTime_;
}
