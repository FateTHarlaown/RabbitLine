//
// Created by NorSnow_ZJ on 2018/9/25.
//
#include <iostream>
#include "rabbitline.h"
#include <functional>

void timerFunc(int n, std::string msg)
{
    std::cout << "Timer: " << n << "炸了啊!它说: " << msg << std::endl;
}

void stopAll()
{
    std::cout << "全都闭嘴！！！" << std::endl;
    RabbitLine::stopLoop();
}

int main()
{
    RabbitLine::addTimer(RabbitLine::Timestamp::nowAfterMilliSeconds(2000),
                         std::bind(timerFunc, 110, "你被捕了！"), true, 1000);
    RabbitLine::addTimer(RabbitLine::Timestamp::nowAfterMilliSeconds(3000),
                         std::bind(timerFunc, 120, "你要开刀了！"), true, 3000);
    RabbitLine::addTimer(RabbitLine::Timestamp::nowAfterMilliSeconds(20000),
                         stopAll);
    RabbitLine::eventLoop();

    std::cout << "现在一切都安静了！" << std::endl;

    return 0;
}

