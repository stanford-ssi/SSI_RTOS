#pragma once

#include <FreeRTOS.h>
#include "MsgBuffer.hpp"

#include "Task.hpp"

#include "AltFilter.hpp"
#include "FlightPlan.hpp"
#include "SensorData.h"

class BuzzerTask : public Task<2000>
{
public:
    BuzzerTask(uint8_t priority);

private:
    void activity();
};

#include "main.hpp"
#include "ArduinoJson.h"
