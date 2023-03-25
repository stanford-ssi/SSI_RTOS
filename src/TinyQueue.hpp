#pragma once


template <typename T, int sz>
class TinyQueue
{
private:
    T tQueue[sz];
    uint32_t begin;
    uint32_t end;
    uint32_t queueSize; 

public:
    TinyQueue() {} 

    void printTinyQueue() {
        Serial.print("----queue size: ");
        Serial.println(queueSize);
        for (int i = 0; i < queueSize; i++) {
            // Serial.print(" ");
            Serial.write(tQueue[(begin+i)%queueSize]);
        }
        Serial.println();
    }

    
    void push(T taskid)
    {
        T *curptr = tQueue + end;
        *curptr = taskid;
        end += 1;
        if (end == sz) {
            end = 0;
        }
        if (end == begin) {
            begin += 1;
        }
        queueSize++;
    }

    T pop()
    {
        if (queueSize > 0) {
            T* curptr = tQueue + begin;
            begin += 1;
            if (begin == sz) {
                begin = 0;
            }
            queueSize--;
            return *curptr;
        }
        return tQueue[0];
    }

    uint32_t size() {
        return queueSize;
    }
};