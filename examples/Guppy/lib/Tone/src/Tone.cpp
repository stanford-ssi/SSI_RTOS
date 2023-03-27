#include "Tone.h"
#include "compiler.h"
#include "Arduino.h"
#include "wiring_private.h"

#define WAIT_TC16_REGS_SYNC(x) while(x->COUNT16.SYNCBUSY.bit.ENABLE);

Tone::Tone(uint32_t pin){

  _pin = pin;
  //set clock controller to send a clock signal to the TC logic
  GCLK->PCHCTRL[TC3_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);
  //enable mclk mask so that we can talk to the device
  hri_mclk_set_APBBMASK_TC3_bit(MCLK);
  
  //disable the TC
  TC3->COUNT16.CTRLA.bit.ENABLE = false;
  WAIT_TC16_REGS_SYNC(TC3)
  
  //reset the TC
  TC3->COUNT16.CTRLA.bit.SWRST = true;
  WAIT_TC16_REGS_SYNC(TC3)
  while (TC3->COUNT16.CTRLA.bit.SWRST);

  //set the settings we want
  TC3->COUNT16.CTRLA.bit.MODE = TC_CTRLA_MODE_COUNT16_Val;
  TC3->COUNT16.CTRLA.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV8_Val;
	TC3->COUNT16.WAVE.bit.WAVEGEN = TC_WAVE_WAVEGEN_MFRQ_Val;

}

void Tone::set(uint32_t frequency)
{
  if(frequency){
    pinMode(_pin, OUTPUT);
    pinPeripheral(_pin, PIO_TIMER);
    uint16_t count = F_CPU / 8 / frequency;
    TC3->COUNT16.CC[1].reg = count;
    TC3->COUNT16.CC[0].reg = count;
    TC3->COUNT16.CTRLA.bit.ENABLE = true;

  }else{

    TC3->COUNT16.CTRLA.bit.ENABLE = false;
    pinPeripheral(_pin, PIO_DIGITAL);
    digitalWrite(_pin, false);

  }
}
