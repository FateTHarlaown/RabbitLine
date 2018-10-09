//
// Created by NorSnow_ZJ on 2018/10/8.
//

#include <list>
#include <iostream>
#include <unistd.h>
#include "rabbitline.h"
#include "cond.h"

std::list<int> queue;
RabbitLine::cond con;

void producer()
{
    std::cout << "producer start!" << std::endl;
    for (int i = 0; i < 10; ++i)
    {
        queue.push_back(i);
        std::cout << "producer pushed " << i << std::endl;
        con.signal();
        sleep(2);
    }
    std::cout << "producer over!" << std::endl;
    RabbitLine::stopLoop();
}

void consumer()
{
    std::cout << "consumer start!" << std::endl;
    for (int i = 0; i < 10; ++i)
    {
        while (queue.empty()) {
            con.wait();
        }
        int tmp = queue.front();
        queue.pop_front();
        std::cout << "consumer geted " << tmp << std::endl;
    }

    std::cout << "consumer over!!" << std::endl;
}

int main()
{
    RabbitLine::enableHook();
    int64_t cid = RabbitLine::create(consumer);
    RabbitLine::resume(cid);
    int64_t pid = RabbitLine::create(producer);
    RabbitLine::resume(pid);
    RabbitLine::eventLoop();
}

