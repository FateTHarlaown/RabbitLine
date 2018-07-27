#include <iostream>
#include <unistd.h>
#include "coroutline.h"

void func1(arg_t arg)
{
    std::cout << "aaaaaa" << std::endl;
    arg.sc->yeild();
    std::cout << "bbbbbb" << std::endl;
    arg.sc->yeild();
    std::cout << "cccccc" << std::endl;
    arg.sc->yeild();
    std::cout << "dddddd" << std::endl;
}


void func2(arg_t arg)
{
    std::cout << "uuuuuu" << std::endl;
    arg.sc->yeild();
    std::cout << "xxxxxx" << std::endl;
    arg.sc->yeild();
    std::cout << "yyyyyy" << std::endl;
    arg.sc->yeild();
    std::cout << "zzzzzz" << std::endl;
}

int main()
{
    std::cout << "coroutline_t in other thread!" << std::endl;
    Scheduler sc;
    sc.create(func1, NULL);
    sc.create(func2, NULL);
    sc.startLoopInThread();

    sleep(10);

    sc.stopLoop();

    int co1 = sc.create(func1, NULL);
    int co2 = sc.create(func2, NULL);


    std::cout << "coroutline_t in local thread!" << std::endl;
    while (sc.getStatus(co1) !=  FREE || sc.getStatus(co2) != FREE) {
        sc.resume(co1);
        sc.resume(co2);
    }

    return 0;
}