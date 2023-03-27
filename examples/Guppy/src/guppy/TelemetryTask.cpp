#include "TelemetryTask.hpp"
#include "Packet.hpp"
#include "compression_utils.h"

TelemetryTask::TelemetryTask(uint8_t priority) : Task(priority) {}

const char *str(FlightState state)
{
    switch (state)
    {
    case Waiting:
        return "Waiting";
    case Flight:
        return "Flight";
    case Falling:
        return "Falling";
    case Landed:
        return "Landed";
    default:
        return "?";
    }
}

void TelemetryTask::activity()
{
    while (true)
    {
        vTaskDelay(1000);

        if (sys.tasks.radio.isIdle())
        {

            // gps_data_t data;
            // data.lat = 0;
            // data.lon = 0;
            // data.alt = 0;
            // sys.tasks.gps.locationData.get(data, xGetTaskId());
            
            OneBattery::cell_voltage_t batt;
            
            sys.tasks.alt.battData.get(batt, xGetTaskId());
            // batt.cellMain = 0;
            rf_down_t down_packet;

            down_packet.time = trimBits(xTaskGetTickCount() / 1000, 18); //seconds mod 72 hours

            float p_alt_cur = 0;
            float p_vel_cur = 0;
            sys.tasks.filter.filter.p_alt.get(p_alt_cur, xGetTaskId());
            sys.tasks.filter.filter.p_vel.get(p_vel_cur, xGetTaskId());
            
            down_packet.filter_alt = compressFloat(p_alt_cur, -2000.0, 40000.0, 15);
            down_packet.filter_vel = compressFloat(p_vel_cur, -1000.0, 1000.0, 11);

            down_packet.battery = compressFloat(batt.cellMain, 1.0, 6.0, 8);
            down_packet.lat = compressFloat(0, 0.0, 10.0, 18);
            down_packet.lon = compressFloat(0, 0.0, 10.0, 18);
            down_packet.gps_alt = compressFloat(0, -2000.0, 40000.0, 15);

            down_packet.gps_lock = false;
            bool armed_val;
            sys.armed.get(armed_val, xGetTaskId());
            down_packet.armed = armed_val;

            FlightState temp_p_state;
            sys.tasks.filter.plan.p_state.get(temp_p_state, xGetTaskId());
            down_packet.state = temp_p_state;

            down_packet.pyroA = sys.pyro.getStatus(Pyro::SquibA, xGetTaskId());
            down_packet.pyroB = sys.pyro.getStatus(Pyro::SquibB, xGetTaskId());

            bool pA_temp;
            bool pB_temp;
            sys.tasks.filter.plan.pyroA_fired.get(pA_temp, xGetTaskId());
            sys.tasks.filter.plan.pyroB_fired.get(pB_temp, xGetTaskId());
            down_packet.pyroA_fired = pA_temp;
            down_packet.pyroB_fired = pB_temp;
            
            packet_t pkt;
            memcpy(pkt.data, &down_packet, sizeof(down_packet));
            pkt.len = sizeof(down_packet);

            // Serial.println("telemetry task queuing up some telemetry");
            sys.tasks.radio.sendPacket(pkt, xGetTaskId());
        } else {
            // Serial.println("radio not ready?");
            // vTaskDelay(200);
            // sys.tasks.radio.wakeRadio();
        }
    }
}