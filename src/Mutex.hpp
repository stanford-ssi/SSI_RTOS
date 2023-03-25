#pragma once
#include "FreeRTOS.h"
#include "TinyQueue.hpp"

class Mutex{

public:
    Mutex();
    void take(uint32_t taskid);
    void give(String s, uint32_t taskid);
    uint8_t mutexTaken();
    // Mutex(const Mutex&); //custom copy
	// Mutex& operator=(const Mutex&) = delete; //assignment does not make sense

    // Mutex(Mutex&&) = delete; //no such thing as move
    // Mutex& operator=(Mutex&&) = delete; //assignment does not make sense
    void printMutexQueue();
    ~Mutex();

private:
    uint8_t istaken = 0;
    // uint32_t taskQueue[OS_CONFIG_MAX_TASKS];
    TinyQueue<uint32_t, OS_CONFIG_MAX_TASKS> tq;
    uint32_t queueSize = 0;
};