# SSI RTOS
_Rehaan Ahmad, Alex Paek, Raj Palleti_

We provide our own implementation of RTOS that the SpaceSalmon codebase can be built upon instead of FreeRTOS. Like FreeRTOS, task pre-emption is governed by user defined delays, mutexes, event bits, blocking message buffer receives, and more. 

# SSI RTOS Features
Below are a list of features SSI RTOS supports that are needed to support the SSI SpaceSalmon codebase
## Priority delay
To delay a task for a specified number of milliseconds from a specified point, run:
```C
vTaskDelay(uint32_t delay_ticks)
```
This will switch to another task, and after delay_ticks time, will pre-empt back to the calling task if no higher priority task is already running. If a user would like to delay a task until an absolute number of ticks, one can use vTaskDelayUntil like so:
```C
xLastWakeTime = xTaskGetTickCount();
while (true) {
    // Wait for the next cycle in 100 ms
    vTaskDelayUntil( &xLastWakeTime, 100);
    // Perform action here.
}
```
## Mutexes
Individual tasks can use our mutexes by including "Mutex.hpp" in their header file. 

## Posters

## MsgBuffers and StrBuffers

## Event Bit Notifications and Delays

## Critical Sections

## Interrupt Requests

# Context Switching on the M4-Cortex
Context switching on the M4 cortex

# Tests
