//
// Created by NorSnow_ZJ on 2018/8/6.
//

#ifndef COROUTLINE_CALLBACKS_H
#define COROUTLINE_CALLBACKS_H

#include <functional>

using EventCallbackFunc = std::function<void ()>;
using TimeoutCallbackFunc = std::function<void ()>;
using PendingCallbackFunc = std::function<void ()>;

#endif //COROUTLINE_CALLBACKS_H
