#include <stdint.h>
#include <Arduino.h>

#define OS_CONFIG_MAX_TASKS	10
extern "C" void rpi_init_trampoline();


void setTask(void (*code)(void *args), void* args, uint32_t taskpriority, uint32_t* xStack, uint32_t stack_size);
void delayTask(uint32_t delay_ticks, uint32_t taskid);
void delayTaskUntil(uint32_t* lasttime, uint32_t delay_ticks, uint32_t taskid);
uint32_t xTaskGetTickCount();
uint32_t xEventGroupWaitBits(uint32_t taskid, uint8_t uxBitsToWaitFor, bool xClearOnExit, uint32_t time, bool forever);
void startTask();
void begin_critical();
void end_critical();
void set_task(uint32_t taskid, uint8_t ta);
void printActiveTasks();
uint32_t get_tick_counter();
void trigger_switch();
volatile uint32_t get_trigger();
void set_notify_array(uint32_t taskid, uint8_t uxBitsToWaitFor);
void yieldFromISR();
