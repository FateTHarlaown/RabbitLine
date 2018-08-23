#include <iostream>
#include <unistd.h>
#include "coroutline.h"
#include "poller.h"

void func4(void * arg)
{
    std::cout << "ggggggg" << std::endl;
}

void func3(void * arg)
{
    Scheduler * sc = getLocalScheduler();
    std::cout << "mmmmmmm" << std::endl;
    int c4 = sc->create(func4, NULL);
    sc->resume(c4);
    std::cout << "ooooooo" << std::endl;
}

void func2(void * arg)
{
    Scheduler * sc = getLocalScheduler();
    int c3 = sc->create(func3, NULL);
    sc->resume(c3);
    std::cout << "uuuuuu" << std::endl;
    sc->yield();
    std::cout << "zzzzzz" << std::endl;
}

void func1(void * arg)
{
    Scheduler * sc = getLocalScheduler();
    std::cout << "aaaaaa" << std::endl;
    int c2 = sc->create(func2, NULL);
    sc->resume(c2);
    std::cout << "dddddd" << std::endl;
    sc->resume(c2);
    std::cout << "xxxxxxx" << std::endl;
}



void timer1()
{
    std::cout << "Timer1 爆了啊！！！" << std::endl;
}

void timer2()
{
    std::cout << "Timer2 炸了啊！！！！！！！！！" << std::endl;
}

int main()
{

    std::cout << "coroutline_t" << std::endl;
    Scheduler * sc = getLocalScheduler();
    int c1 = sc->create(func1, NULL);
    sc->resume(c1);
    /*
    Poller * p = new PollPoller();
    p->addTimer(Timestamp::nowAfter(1), timer1, true, 3);
    p->addTimer(Timestamp::nowAfter(5), timer2, true, 5);
    while (1) {
        p->runPoll();
    }
     */
    sc->mainLoop();
    return 0;
}