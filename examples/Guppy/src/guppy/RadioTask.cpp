#include "RadioTask.hpp"
#include "main.hpp"
#include <rBase64.h>

RadioTask * RadioTask::glob_ptr = nullptr;

void RadioTask::radioISR(void)
{
    // Serial.println("radio task interrupt...");
    set_task(glob_ptr->objtaskid, 1);
    set_notify_array(glob_ptr->objtaskid, 0b01);
    trigger_switch();
    yieldFromISR();
}

RadioTask::RadioTask(uint8_t priority) : Task(priority)
{
    glob_ptr = this;
    objtaskid = priority;
}

bool RadioTask::isIdle(){
    return txbuf.empty();
}

bool RadioTask::sendPacket(packet_t &packet, uint32_t taskid)
{
    bool ret = txbuf.send(packet, taskid);     //this puts the packet in the buffer
    set_task(glob_ptr->objtaskid, 1);
    set_notify_array(glob_ptr->objtaskid, 0b10);
    trigger_switch();
    while(get_trigger() == 1) {
        __NOP();
    }
    return ret;
}

void RadioTask::waitForPacket(packet_t &packet, uint32_t taskid)
{
    rxbuf.receive(packet, taskid, true);
}

void RadioTask::applySettings(radio_settings_t &settings)
{
    lora.setFrequency(settings.freq);
    lora.setBandwidth(settings.bw);
    lora.setSpreadingFactor(settings.sf);
    lora.setCodingRate(settings.cr);
    lora.setSyncWord(settings.syncword);
    lora.setOutputPower(settings.power);
    lora.setCurrentLimit(settings.currentLimit);
    lora.setPreambleLength(settings.preambleLength);
    lora.setTCXO(settings.TcxoVoltage);
}

void RadioTask::setSettings(radio_settings_t &settings)
{
    settingsBuf.send(settings, 0);
}

void RadioTask::wakeRadio() {
    set_task(glob_ptr->objtaskid, 1);
    trigger_switch();
    while(get_trigger() == 1) {
        __NOP();
    }
}

radio_settings_t RadioTask::getSettings()
{
    settingsMx.take(xGetTaskId());
    radio_settings_t temp = settings;
    settingsMx.give("debug", xGetTaskId());
    return temp;
}

void RadioTask::activity()
{
    spi.begin();
    while (true)
    {
        int state = lora.begin(settings.freq, settings.bw, settings.sf, settings.cr, settings.syncword, settings.power, settings.preambleLength, settings.TcxoVoltage, false);

        if (state != ERR_NONE)
        {
            StaticJsonDocument<1000> doc;
            doc["id"] = "radio";
            doc["msg"] = "Init Failed";
            doc["code"] = state;
            doc["tick"] = xTaskGetTickCount();
            sys.tasks.logger.log(doc, xGetTaskId());
            vTaskDelay(1000);
        }
        else
        {
            break;
        }
    }
    // Serial.println("about to set priority");
    NVIC_SetPriority(EIC_15_IRQn, 1);
    lora.setDio1Action(radioISR);
    NVIC_SetPriority(EIC_15_IRQn, 1);

    lora.setDioIrqParams(SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_HEADER_ERR | SX126X_IRQ_CRC_ERR | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CAD_DETECTED | SX126X_IRQ_CAD_DONE,
                         SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_HEADER_ERR | SX126X_IRQ_CRC_ERR | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CAD_DETECTED | SX126X_IRQ_CAD_DONE);

    lora.startReceive();

    uint32_t time = xTaskGetTickCount();
    uint8_t continuebit=  0;
    while (true)
    {
        // Serial.println("radio task tf??");
        //if new settings are available, apply them
        if (!settingsBuf.empty())
        {
            settingsMx.take(xGetTaskId());
            settingsBuf.receive(settings, xGetTaskId(), false);
            applySettings(settings);
            settingsMx.give("str", xGetTaskId());
        }

        if (xTaskGetTickCount() - time > 15000)
        {
            logStats();
            time = xTaskGetTickCount();
        }

        // uint32_t flags = xEventGroupWaitBits(evgroup, 0b11, true, false, NEVER);
        // Serial.println("waiting here fool?");
        // uint32_t flags = 0;
        // Serial.println("trying to wait");
        uint32_t flags = xEventGroupWaitBits(xGetTaskId(), 0b11, true, time, true);
        // Serial.println("got past first wait");
        // xEventGroupWaitBits(xGetTaskId(), 0, true);        
        uint16_t irq = lora.getIrqStatus();
        lora.clearIrqStatus();

        // Serial.println("about to process Tx and Rx?");
        //RECEIVE BLOCK
        if (irq & SX126X_IRQ_PREAMBLE_DETECTED)
        {
            log(info, "Preamble");
            // Serial.println("preamble");
            uint32_t time = lora.symbolToMs(32); //time of a packet header

            // flags = xEventGroupWaitBits(evgroup, 0b01, true, false, time); //wait for another radio interupt for 500ms
            // flags = xEventGroupWaitBits(xGetTaskId(), time, false);
            flags = xEventGroupWaitBits(xGetTaskId(), 0b01, true, time, false);

            if (flags == 0)
            {
                log(warning, "Missed Header");
                // Serial.println("missed header");
                rx_failure_counter++;
                goto tx_block;
                //continue; //it failed
            }

            irq = lora.getIrqStatus();
            lora.clearIrqStatus();

            if ((irq & SX126X_IRQ_HEADER_VALID) && !(irq & SX126X_IRQ_RX_DONE)) //header is ready, but not packet. We are confident that an RxDone will come, (sucessfully or otherwise)
            {
                log(info, "wait RxDone");
                // Serial.println("wait RxDone");
                uint32_t time = lora.getTimeOnAir(255) / 1000;                 //TODO: read this out of the header to make timeout tighter
                // flags = xEventGroupWaitBits(evgroup, 0b01, true, false, time); //wait for radio interupt for 500ms
                // flags = xEventGroupWaitBits(xGetTaskId(), time, false);
                flags = xEventGroupWaitBits(xGetTaskId(), 0b01, true, time, false);
                if (!(flags & 0b01))
                {
                    log(warning, "Missed RxDone");
                    // Serial.println("missed rx done");
                    rx_failure_counter++;
                    goto tx_block;
                    //continue; //it failed
                }

                irq = lora.getIrqStatus();
                lora.clearIrqStatus();
            }

            if (irq & SX126X_IRQ_RX_DONE) //packet is ready, we can grab it
            {
                log(info, "RxDone");
                // Serial.println("RxDone");
                if ((irq & SX126X_IRQ_CRC_ERR) || (irq & SX126X_IRQ_CRC_ERR)) {
                    log(warning, "CRC Error");
                    // Serial.println("CRC error");
                }

                packet_t packet;
                lora.readData(packet.data, 255);
                packet.len = lora.getPacketLength();
                packet.rssi = lora.getRSSI();
                packet.snr = lora.getSNR();
                rxbuf.send(packet, 0);
                logPacket("RX", packet);
                // Serial.println("RX packet");
                rx_success_counter++;
                
            }
            else
            {
                // Serial.println("--Missed RxDone");
                log(warning, "Missed RxDone");
                rx_failure_counter++;
                goto tx_block;
            }
        }
        
        tx_block:
        // Serial.println("trying to tx");
        // Serial.println(txbuf.empty());
        //TRANSMIT BLOCK
        if (txbuf.empty())
        {
            log(info, "keep RXing");
            // Serial.println("-- keep rxing");
            lora.startReceive();
            continue;
        }
        else
        {
            log(info, "Time to TX");
            // Serial.println("--time to tx");
            do
            {
                // Serial.println("got to the rx");
                log(info, "Wait quiet");
                // Serial.println("--wait quiet");
                lora.standby();
                lora.setCad();
                uint32_t time = lora.symbolToMs(12) + 50;
                // xEventGroupWaitBits(evgroup, 0b01, true, false, time);
                // xEventGroupWaitBits(xGetTaskId(), time, false);
                xEventGroupWaitBits(xGetTaskId(), 0b01, true, time, false);
                irq = lora.getIrqStatus();
                lora.clearIrqStatus();
            } while (!(irq & SX126X_IRQ_CAD_DONE) || irq & SX126X_IRQ_CAD_DETECTED);

            log(info, "Heard quiet");
            // Serial.println("--heard quiet");
            packet_t packet;
            if (txbuf.receive(packet, xGetTaskId(), false))
            {

                uint32_t time = lora.getTimeOnAir(packet.len) / 1000; //get TOA in ms
                time = (time * 1.1) + 100;                            //add margin

                logPacket("TX", packet);

                // Serial.println("--TX");
                lora.startTransmit(packet.data, packet.len);
                // flags = xEventGroupWaitBits(evgroup, 0b01, true, false, time);
                // flags = xEventGroupWaitBits(xGetTaskId(), time, false);
                flags = xEventGroupWaitBits(xGetTaskId(), 0b01, true, time, false);
                // delay(time);
                // Serial.print("flag value: ");
                // Serial.println(flags);
                // Serial.println(time);
                irq = lora.getIrqStatus();
                lora.clearIrqStatus();

                if (irq & SX126X_IRQ_TX_DONE)
                {
                    log(info, "TXDone");
                    // Serial.println("--TXDone");
                    tx_success_counter++;
                }
                else
                {
                    log(warning, "Missed TXDone");
                    // Serial.println("Missed TxDone");
                    tx_failure_counter++;
                }

                lora.startReceive();
                time = lora.symbolToMs(32);                             //this can be tuned
                // xEventGroupWaitBits(evgroup, 0b01, false, false, time); //wait for other radio to get a chance to speak
                // Serial.print("about to wait for: ");
                // Serial.println(time);
                // xEventGroupWaitBits(xGetTaskId(), time, false);
                xEventGroupWaitBits(xGetTaskId(), 0b01, false, time, false);
                continuebit = 1;
            }
            else
            {
                // Serial.println("TX Quiete Failure");
                log(warning, "TX Queue Failure");
                tx_failure_counter++;
            }
        }
    }
}

void RadioTask::log(log_type t, const char *msg)
{
    if (t & settings.log_mask)
    {
        StaticJsonDocument<1000> doc;
        doc["id"] = "radio";
        doc["msg"] = msg;
        doc["level"] = (uint8_t)t;
        doc["tick"] = xTaskGetTickCount();
        sys.tasks.logger.log(doc, xGetTaskId());
    }
}

void RadioTask::logPacket(const char *msg, packet_t &packet)
{
    if (settings.log_mask & data)
    {
        StaticJsonDocument<1000> doc;
        doc["id"] = "radio";
        doc["msg"] = msg;
        doc["level"] = (uint8_t)data;
        doc["tick"] = xTaskGetTickCount();
        doc["rssi"] = packet.rssi;
        doc["snr"] = packet.snr;
        doc["len"] = packet.len;
        char buf[350];
        rbase64_encode(buf, (char *)packet.data, packet.len);
        doc["data"] = buf;
        sys.tasks.logger.log(doc, xGetTaskId());
    }
}

void RadioTask::logStats()
{
    if (settings.log_mask & stats)
    {
        StaticJsonDocument<1000> doc;
        doc["id"] = "radio";
        doc["msg"] = "stats";
        doc["level"] = (uint8_t)stats;
        doc["tick"] = xTaskGetTickCount();
        doc["rx_success"] = rx_success_counter;
        doc["rx_failure"] = rx_failure_counter;
        doc["tx_success"] = tx_success_counter;
        doc["tx_failure"] = tx_failure_counter;
        doc["freq"] = settings.freq;
        doc["bw"] = settings.bw;
        doc["sf"] = settings.sf;
        doc["cr"] = settings.cr;
        doc["pwr"] = settings.power;
        sys.tasks.logger.log(doc, xGetTaskId());
    }
}