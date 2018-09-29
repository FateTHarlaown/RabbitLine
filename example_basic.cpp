#include <iostream>
#include "rabbitline.h"

void func4()
{
    std::cout << "ggggggg" << std::endl;
}

void func3()
{
    std::cout << "mmmmmmm" << std::endl;
    int64_t c4 = RabbitLine::create(func4);
    RabbitLine::resume(c4);
    std::cout << "ooooooo" << std::endl;
}

void func2()
{
    int64_t c3 = RabbitLine::create(func3);
    RabbitLine::resume(c3);
    std::cout << "uuuuuu" << std::endl;
    RabbitLine::yield();
    std::cout << "zzzzzz" << std::endl;
}

void func1()
{
    std::cout << "aaaaaa" << std::endl;
    int64_t c2 = RabbitLine::create(func2);
    RabbitLine::resume(c2);
    std::cout << "dddddd" << std::endl;
    RabbitLine::resume(c2);
    std::cout << "xxxxxxx" << std::endl;
}


int main()
{

    std::cout << "coroutline_t" << std::endl;
    int64_t c1 = RabbitLine::create(func1);
    RabbitLine::resume(c1);

    return 0;
}