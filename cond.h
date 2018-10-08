//
// Created by NorSnow_ZJ on 2018/10/8.
//

#ifndef RABBITLINE_COND_H
#define RABBITLINE_COND_H

#include <list>
#include "coroutline.h"

namespace RabbitLine
{

class cond
{
public:
    void wait();

    void timewait(uint ms);

    void signal();

    void broadcast();

private:
    std::list<int64_t> waitList_;
};

}
#endif //COROUTLINE_COND_H
