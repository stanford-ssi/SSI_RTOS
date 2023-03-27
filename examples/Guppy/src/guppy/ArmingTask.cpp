#include "ArmingTask.hpp"

ArmingTask::ArmingTask(uint8_t priority) : Task(priority){}

void ArmingTask::activity(){
    while(true){
        packet_t rx_pkt;
        sys.tasks.radio.waitForPacket(rx_pkt, xGetTaskId());
        
        if(rx_pkt.data[0] == 'A'){
            sys.armed.post(true, xGetTaskId());
        }else if (rx_pkt.data[0] == 'D'){
            sys.armed.post(false, xGetTaskId());
        }
    }
}