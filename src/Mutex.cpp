#include "Mutex.hpp"

Mutex::Mutex(){
}

void Mutex::take(uint32_t taskid){
    begin_critical();
    if (istaken == 0) {
        istaken = 1;
    } else {
        while (istaken == 1) {
            tq.push(taskid);
            queueSize += 1;
            set_task(taskid, 0);
            end_critical();
            trigger_switch();

            // wait till the switch actually happened
            while(get_trigger() == 1) {
                __NOP();
            }
            begin_critical();
        }
        if (istaken == 1) {
            Serial.println("wtf??? mutex still taken??");
        }
        istaken = 1;
    }
    end_critical();
}

uint8_t Mutex::mutexTaken() {
    return istaken;
}

void Mutex::give(String s, uint32_t taskid){
    begin_critical();
    if (istaken == 0) {
        Serial.println(s);
        printMutexQueue();
        Serial.println("this mutex code is not working! Task id: ");
        Serial.println(taskid);
    }
    istaken = 0;
    if (queueSize > 0) {
        uint32_t readyTaskId = tq.pop();
        set_task(readyTaskId, 1);
        queueSize -= 1;
        end_critical();
        trigger_switch();
        while(get_trigger() == 1) {
            __NOP();
        }
    }
    end_critical();
}

void Mutex::printMutexQueue() {
    tq.printTinyQueue();
}

Mutex::~Mutex(){
    give("", 20);
}

