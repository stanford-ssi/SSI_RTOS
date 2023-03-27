#pragma once

#include <FreeRTOS.h>
#include "MsgBuffer.hpp"
#include "Poster.hpp"
#include "Task.hpp"

class TelemetryTask : Task<2000>
{
public:
    TelemetryTask(uint8_t priority);

private:

    void activity();
};

#include "main.hpp"
#include "ArduinoJson.h"
