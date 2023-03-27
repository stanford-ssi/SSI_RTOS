#include "ssi_adc.h"
#include "compiler.h"
#include "hal/include/hpl_adc_sync.h"

ADC::ADC(Adc* hw_addr){
  hw = hw_addr;
  hri_mclk_set_APBDMASK_ADC0_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, ADC0_GCLK_ID, 0 | (1 << GCLK_PCHCTRL_CHEN_Pos)); //Generator 0 = 120Mhz
  _adc_generic_init(hw);//ASF initialization code
}

uint16_t ADC::read(uint8_t pin, uint32_t taskid){

  mx.take(taskid);

  uint32_t valueRead;
  
  while( hw->SYNCBUSY.reg & ADC_SYNCBUSY_INPUTCTRL ); //wait for sync
  hw->INPUTCTRL.bit.MUXPOS = pin;
  
  while( hw->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE ); //wait for sync
  hw->CTRLA.bit.ENABLE = 0x01;             // Enable ADC

  // Start conversion
  while( hw->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE ); //wait for sync
  
  hw->SWTRIG.bit.START = 1;

  // Clear the Data Ready flag
  hw->INTFLAG.reg = ADC_INTFLAG_RESRDY;

  // Start conversion again, since The first conversion after the reference is changed must not be used.
  hw->SWTRIG.bit.START = 1;

  // Store the value
  while (hw->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete
  valueRead = hw->RESULT.reg;

  while( hw->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE ); //wait for sync
  hw->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  while( hw->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE ); //wait for sync*/

  // mx.printMutexQueue();
  mx.give("ssi adc", taskid);
  return valueRead;
}