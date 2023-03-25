#pragma once
#include "Mutex.hpp"
#include "TinyQueue.hpp"
/*
Poster is a class template that allows safe access to a single variable in a post()/get() style.
Use this if you want to provide a way to access the most recent value of a peice of data, but dont need every sample (in that case, you would use a buffer).
This is useful for telemetry and diagnostic indicators. 
*/

// sz is defined in bytes
template <typename T, int sz>
class MsgBuffer
{
private:
    TinyQueue<T, (sz/sizeof(T))> storageQ;
    uint32_t queueSize = 0; //in T
    // before you increment either of these pointers, check if they are the max size
    TinyQueue<uint32_t, OS_CONFIG_MAX_TASKS> tq;
public:
    MsgBuffer() {} 

    void printActiveQueue() {
        storageQ.printTinyQueue();
    }

    bool empty()
    {
        if (queueSize == 0) {
            return true;
        }
        return false;
    }

    uint32_t send(T &data, uint32_t taskid)
    {
        begin_critical();
        storageQ.push(data);
        queueSize++;

        if (tq.size() > 0) {
            set_task(tq.pop(), 1);
            trigger_switch();
        }
        end_critical();
        
        return 1;
    }

    u_int32_t receive(T &data, uint32_t taskid, bool block)
    {
        begin_critical();
        if (block == true) {
            if (queueSize == 0) {
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
            if (queueSize == 0) {
                end_critical();
                return 0;
            }
        }

        T curval = storageQ.pop();
        data = curval;
        queueSize -= 1;
        end_critical();
        return 1;
    }
};