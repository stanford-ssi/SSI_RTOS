#pragma once
#include "FreeRTOS.h"

template <int stackSize>
class Task { // This class is linked to EACH Task#.hpp/cpp file! As it is the parent of all these subclasses
    private:
        // TaskHandle_t taskHandle;
        // StaticTask_t xTaskBuffer;
        uint32_t xStack[stackSize];
        uint8_t taskpriority;

    public:
        Task(uint8_t priority) {
            taskpriority = priority; 
            setTask(Task::TaskFunctionAdapter, this, taskpriority, xStack, stackSize);
        }

        // A "virtual" function means it must be implemented by the child class (Ex. Task1, Task2)
        virtual void activity()
        {
        };

        void suspend(void){
        }

        void resume(void){
        }

        uint32_t xGetTaskId() {
            return taskpriority;
        }
        
        void vTaskDelay(uint32_t delay_ticks) {
            delayTask(delay_ticks, taskpriority);
        }

        void vTaskDelayUntil(uint32_t* lasttime, uint32_t delay_ticks) {
            delayTaskUntil(lasttime, delay_ticks, taskpriority);
        }
        
        static void TaskFunctionAdapter(void *rvParams)
        {
            Task *task = static_cast<Task *>(rvParams);
            task->activity();
        }
};