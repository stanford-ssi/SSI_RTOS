#pragma once

#include "ssi_adc.h"
#include <stdint.h>

class OneBattery
{
  private:
    ADC& _adc;
  public:

    struct cell_voltage_t
    {
        float cellMain;
        float cellBackup;
    };
    
    OneBattery(ADC& adc);
    cell_voltage_t readVoltage(uint32_t taskid);
};