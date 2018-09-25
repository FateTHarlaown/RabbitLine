//
// Created by NorSnow_ZJ on 2018/8/6.
//

#ifndef RABBITLINE_CALLBACKS_H
#define RABBITLINE_CALLBACKS_H

#include <functional>

namespace RabbitLine {

using EventCallbackFunc = std::function<void ()>;
using TimeoutCallbackFunc = std::function<void ()>;
using PendingCallbackFunc = std::function<void ()>;

}

#endif //RABBITLINE_CALLBACKS_H

