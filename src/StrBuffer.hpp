#pragma once
#include "Mutex.hpp"
#include "TinyQueue.hpp"
/*
Poster is a class template that allows safe access to a single variable in a post()/get() style.
Use this if you want to provide a way to access the most recent value of a peice of data, but dont need every sample (in that case, you would use a buffer).
This is useful for telemetry and diagnostic indicators. 
*/

// sz is defined in bytes
template <int sz>
class StrBuffer
{
private:
    TinyQueue<uint8_t, sz> storageQ;
    uint32_t queueSize = 0; //in T
    // before you increment either of these pointers, check if they are the max size
    TinyQueue<uint32_t, OS_CONFIG_MAX_TASKS> tq;

public:
    StrBuffer() {} 

    void printActiveQueue() {
        storageQ.printTinyQueue();
    }
    uint32_t send(const char *data, size_t len, uint32_t taskid)
    {
        begin_critical();
        for (unsigned int i = 0; i < len; i++) {
            storageQ.push(*(data+i));
            queueSize++;
        }
        if (tq.size() > 0) {
            set_task(tq.pop(), 1);
            trigger_switch();
        }
        end_critical();
        
        return 0;
    }

    u_int32_t receive(char* data, size_t len, uint32_t taskid, bool block)
    {
        begin_critical();
        if (block == true) {
            while (len > queueSize) {
                // this whole thing might need to be a critical section
                set_task(taskid, 0);
                tq.push(taskid);
                end_critical();
                trigger_switch();
                while(get_trigger() == 1) {
                    __NOP();
                }
                begin_critical();
            }
        } else {
            if (len > queueSize) {
                end_critical();
                return 0;
            }
        }
        for (int i = 0; i < len; i++) {
            uint8_t curval = storageQ.pop();
            char* dataptr = data+i;
            *dataptr = curval;
            queueSize -= 1;
            if (curval == 0) {
                end_critical();
                return 1;
            }
        }
        end_critical();
        
        return len;
    }
};