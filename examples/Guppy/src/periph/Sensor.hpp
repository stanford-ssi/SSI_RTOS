#pragma once

enum SensorStatus
{
    Init,
    Alive,
    Error
};

typedef unsigned long uint32_t;
class Sensor
{
public:
    Sensor(const char* newId);
    void readData(); //read data from sensor, store it internally
    SensorStatus getStatus(); //get the status of the sensor
    virtual int init(uint32_t tid) = 0; //initialize the sensor
    const char* getID();
protected:
    char id[5];
};

