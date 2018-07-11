#include <iostream>
#include <unistd.h>
#include "coroutline.h"

void func1(arg_t arg)
{
    std::cout << "aaaaaa" << std::endl;
    arg.sc->yeild();
    std::cout << "bbbbbb" << std::endl;
}


void func2(arg_t arg)
{
    std::cout << "cccccc" << std::endl;
    arg.sc->yeild();
    std::cout << "dddddd" << std::endl;
}

int main()
{
    std::cout << "Hello, coroutline!" << std::endl;
    scheduler sc;
    sc.create(func1, NULL);
    sc.create(func2, NULL);
    sc.start();

    sleep(6);

    sc.stop();
    return 0;
}