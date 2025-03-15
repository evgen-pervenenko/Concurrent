#pragma once

#include <functional>

namespace Async
{
    void WaitAllTasksFinished();
    void Run(std::function<void()> &&);
}