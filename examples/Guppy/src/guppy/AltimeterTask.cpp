#include "AltimeterTask.hpp"

// extern const char *build_version;

AltimeterTask::AltimeterTask(uint8_t priority) : Task(priority) {}

void AltimeterTask::activity()
{
    char str[150];

    // snprintf(str, sizeof(str), "Altimeter Started\nBuild Version: test rtos");
    // Serial.println("Altimeter started Build Version test RTOS");
    // sys.tasks.logger.log(str);

    uint32_t lastStatusTime = xTaskGetTickCount();

    OneBattery battery(sys.adc0);

    while (true)
    {
        // Serial.println("hello delay?");
        vTaskDelayUntil(&lastStatusTime, 1000);
        // Serial.println("after delay");
        digitalWrite(ALT_LED, true);

        StaticJsonDocument<1000> status_json;

        status_json["tick"] = xTaskGetTickCount();

        // uint32_t runtime;
        // TaskStatus_t tasks[15];
        // uint8_t count = uxTaskGetSystemState(tasks, 15, &runtime);

        // JsonObject tasks_json = status_json.createNestedObject("tasks");

        // for (uint8_t i = 0; i < count; i++)
        // {
        //     float percent = ((float)tasks[i].ulRunTimeCounter) / ((float)runtime) * 100.0;
        //     tasks_json[tasks[i].pcTaskName] = percent;
        // }

        OneBattery::cell_voltage_t voltage = battery.readVoltage(xGetTaskId());
        sys.tasks.alt.battData.post(voltage, xGetTaskId());

        JsonObject bat_json = status_json.createNestedObject("bat");
        bat_json["main"] = voltage.cellMain;
        bat_json["bkp"] = voltage.cellBackup;

        status_json["log"] = sys.tasks.logger.isLoggingEnabled();
        
        bool armed;
        sys.armed.get(armed, xGetTaskId());
        status_json["armed"] = armed;

        JsonArray pyro_json = status_json.createNestedArray("pyro");

        bool pyroA = sys.pyro.getStatus(Pyro::SquibA, xGetTaskId());
        bool pyroB = sys.pyro.getStatus(Pyro::SquibB, xGetTaskId());

        pyro_json.add(pyroA);
        pyro_json.add(pyroB);

        if (!sys.shitl) {
            sys.tasks.logger.logJSON(status_json, "status", xGetTaskId());
        }
        digitalWrite(ALT_LED, false);
    }
}
