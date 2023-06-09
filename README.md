# SSI RTOS
_Rehaan Ahmad, Alex Paek, Raj Palleti_

We provide our own implementation of RTOS that the SpaceSalmon codebase can be built upon instead of FreeRTOS. Like FreeRTOS, task pre-emption is governed by user defined delays, mutexes, event bits, blocking message buffer receives, and more. 

# SSI RTOS Features
Below are a list of features SSI RTOS supports that are needed to support the SSI SpaceSalmon codebase. To see an example of how our RTOS library is being used in the flight computer codebase, look under the examples folder. 
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
Individual tasks can use our mutexes by including "Mutex.hpp" in their header file. You can then lock/unlock the mutex by calling give()/take() respectively:
Task 1:
```C
mutex.take(xGetTaskId())
// do mutex protected code in task 1
mutex.give()
```
Task 2:
```C
sys.tasks.task1.mutex.take(xGetTaskId())
// do mutex protected code in task 2
sys.tasks.task1.mutex.give()
```
Regardless of the priority of the tasks, if either task 1 or task 2 is in position of the mutex, our RTOS scheduler will put the task waiting on the mutex to sleep until the mutex is ready to be taken. 

## Posters
Posters are a simple abstraction built on top of mutexes to easily allow for thread-safe shared variables. 
Task 1:
```C
Poster<bool> poster;
void Task1::activity() {
    while (true) {
        ...
        bool curval = true;
        poster.post(curval, xGetTaskId());
        ...
    }
}
```
Task 2:
```C
void Task2::activity() {
    while (true) {
        ...
        bool curval;
        sys.tasks.Task1.poster.get(curval, xGetTaskId());
        // value of Task1 poster is loaded into curval
    }
}
```
## MsgBuffers and StrBuffers
MsgBuffers are a queue used to send information between tasks. A task can send an object to the buffer, while another task can wait to receive an object from that buffer. The receive call can be either blocking or non-blocking -- in blocking mode, our RTOS scheduler will sleep the waiting thread and pre-empt back to it only when there is data ready to be popped from the buffer. Multiple tasks are allowed to send data to the message buffer, but only one task is allowed to receive from the message buffer. 
Task 1: 
```C
msgbuf.send(object, xGetTaskID());
```

Task 2: 
```C
Object curobj;
msgbuf.receive(curobj, xGetTaskID(), blocking=true);
```
StrBuffers are similar to MsgBuffers, except a task can send or receive a variable number of bytes from the queue. The StrBuffer is also specifically designed with storing lines of characters in mind. It is used by the LoggerTask in SpaceSalmon. 

## Event Bit Notifications and Delays
We also support event bit notifications and delays on such notifications. Each task has an 8-bit event field that it can either update or wait on. A task can update the notification bits for a specific task by calling "void set_notify_array(uint32_t taskid, uint8_t uxBitsToWaitFor)". A task can then wait for a set number of bits by calling "xEventGroupWaitBits(...)". xEventGroupWaitBits can wait indefinitely for the bits to be set or for a set amount of time: if xEventGroupWaitBits resumes the task function due to the correct event bits being set, then it will return 1. Event bit notifications are especially useful within the RadioTask:

```C
radioISR(void) {
    ...
    // notify the first event bit of the radio task
    set_notify_arr(currentRadioID, 0b01);
    ...
}

bool RadioTask::sendPacket(packet_t &packet, uint32_t taskid) {
    ...
    // after adding packet to buffer, notify the second bit of the radio task
    set_notify_arr(currentRadioID, 0b10);
}

void RadioTask::activity() {
    while (true) {
        ...
        // before moving on, wait indefinitely for the first two event bits to be set
        xEventGroupWaitBits(xGetTaskId(), 0b11, true, -1, true);
        ...
    }
}
```

## Critical Sections
A user can specify a critical section of a task with begin_critical()/end_critical() if that section of the task must not be disrupted. If an interrupt happens to occur during a critical section, our RTOS scheduler will ensure that it will continue running the critical section code after the interrupt. 

## Interrupt Requests
To return from an interrupt to regularly-scheduled RTOS tasks, call yieldFromISR() at the end of the ISR handler function. This will take care of resuming the proper RTOS task, and not interrupting a critical section. 

# Context Switching on the M4-Cortex
Context switching with the M4 cortex differs from the ARM 1176 in a few ways. Unlike 1176, the only banked register on the Cortex is the SP register. The M4 cortex documentation states that the hardware automatically saved the caller-saved registers during an interrupt or any function call, meaning the only banked register we need is the SP register. This allows us to run code in one of two modes: MSP (main stack pointer) or PSP (process stack pointer) mode. Interrupt handling, default initial setting, etc is all in MSP mode, and so it is our RTOS's responsiblity to ensure proper switching back to PSP mode when pre-empting between tasks. During context switching, it is also the RTOS responsiblity to switch back to floating point unit (FPU) mode as well, if the task was running in FPU mode before pre-emption. 

# Testing
When running in non-SHITL mode, we test that the sensor polling indeed happens every 10 ms. Leaving the flight computer on for 10 min, we log all the queued sensor data to the SD card, and then run a script through the text file ensuring that every single sensor log is exactly 10 ticks apart, no exceptions.

Additionally, we run Some Hardware In The Loop (SHITL) testing where sensor data is queued from a text file collected during a prior rocket launch, as opposed to polling live sensor data from the board. We can then compare the results of filtered altitude and velocity with FreeRTOS vs our RTOS and see that they are virtually identical below:
<p align="center">
  <img src="images/shitl.png" width="550" />
</p>

And as observed from the radio packets that the ground station received:
<p align="center">
  <img src="images/shitlground.png" width="550" />
</p>
