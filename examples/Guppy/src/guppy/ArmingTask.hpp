#pragma once

#include <FreeRTOS.h>
#include "MsgBuffer.hpp"

#include "Task.hpp"

class ArmingTask : Task<2000>
{
public:
    ArmingTask(uint8_t priority);

private:

    void activity();
};

#include "main.hpp"