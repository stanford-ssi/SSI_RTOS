#include "LoggerTask.hpp"
#include "main.hpp"

StrBuffer<10000> LoggerTask::strBuffer;

char LoggerTask::lineBuffer[10000];
char LoggerTask::inputLineBuffer[1000];

bool LoggerTask::loggingEnabled = false;
bool LoggerTask::shitlEnabled = false;

FATFS LoggerTask::fs;
FIL LoggerTask::file_object;
FIL LoggerTask::shitl_file_object;

LoggerTask::LoggerTask(uint8_t priority) : Task(priority) {}

void LoggerTask::log(const char *message, uint32_t tid)
{
    strBuffer.send(message, strlen(message) + 1, tid);
}

void LoggerTask::logJSON(JsonDocument &jsonDoc, const char *id, uint32_t tid)
{
    jsonDoc["id"] = id;
    // jsonDoc["stack"] = uxTaskGetStackHighWaterMark(NULL); //TODO: Check this for capacity... (dangerous!)

    if (jsonDoc.getMember("tick") == NULL)
    {
        jsonDoc["tick"] = xTaskGetTickCount();
    }

    //jsonDoc["la"] = xMessageBufferSpaceAvailable(bufferHandle);

    size_t len = measureJson(jsonDoc);
    char str[len + 5]; //plenty of room!
    serializeJson(jsonDoc, str, sizeof(str));
    log(str, tid);
}

void LoggerTask::log(JsonDocument &jsonDoc, uint32_t tid)
{
    // jsonDoc["stack"] = uxTaskGetStackHighWaterMark(NULL); //TODO: Check this for capacity... (dangerous!)

    if (jsonDoc.getMember("tick") == NULL)
    {
        jsonDoc["tick"] = xTaskGetTickCount();
    }

    size_t len = measureJson(jsonDoc);
    char str[len + 5]; //plenty of room!
    serializeJson(jsonDoc, str, sizeof(str));
    log(str, tid);
}

void LoggerTask::format()
{
    FRESULT res = FR_OK;
    // printf("-I- Format disk %d\n\r", 0);
    // printf("-I- Please wait a moment during formatting...\n\r");
    //res = f_mkfs("0", FM_EXFAT, 512);
    // printf("-I- Disk format finished !\n\r");
    if (res != FR_OK)
    {
        // printf("-E- f_mkfs pb: 0x%X\n\r", res);
    }
}

void LoggerTask::activity()
{
    digitalWrite(DISK_LED, true);
    FRESULT res;

    SSISD sd;
    sd.init();

    //Clear file system object
    memset(&fs, 0, sizeof(FATFS));

    res = f_mount(&fs, "", 1);
    if (res != FR_OK)
    {
        loggingEnabled = false;
        // sys.tasks.logger.log("Could Not Mount Disk", xGetTaskId());
        // Serial.println(res);
    }
    else
    {
        loggingEnabled = true;
        sys.tasks.logger.log("Mounted SD card", xGetTaskId());
    }

    char file_name[20];

    if (loggingEnabled)
    {
        int lognum = 0;
        while (true)
        {
            snprintf(file_name, sizeof(file_name), "log%u.txt", lognum);
            res = f_stat(file_name, NULL);

            if (res == FR_NO_FILE)
            {
                break;
                //found an open file name we can use
            }

            if (res != FR_OK)
            {
                sys.tasks.logger.log("Error stat-ing file", xGetTaskId());
                loggingEnabled = false;
                break;
            }

            lognum++;
        }
    }

    if (loggingEnabled)
    {
        //printf("Logging to file: %s\n", file_name);
        sys.tasks.logger.log("Trying to open log file", xGetTaskId());

        res = f_open(&file_object, file_name, FA_CREATE_ALWAYS | FA_WRITE);
        if (res != FR_OK)
        {
            sys.tasks.logger.log("Failed to Open File", xGetTaskId());
            loggingEnabled = false;
        }

        //SHITL-----
        if (sys.shitl)
        {
            sys.tasks.logger.log("SHITL from file: shitl.txt", xGetTaskId());

            res = f_open(&shitl_file_object, "shitl.txt", FA_READ);
            if (res == FR_OK)
            {
                shitlEnabled = true;
                sys.tasks.logger.log("Starting in SHITL Mode", xGetTaskId());
            }
            else
            {
                shitlEnabled = false;
                sys.tasks.logger.log("SHITL Read Error", xGetTaskId());
            }
        }
    }

    digitalWrite(DISK_LED, false);

    char *p = lineBuffer;
    uint32_t timeout = 0;
    uint32_t shitltimer = xTaskGetTickCount();
    while (true)
    {
        //Step 1: read in all the logs
        // begin_critical();
        // Serial.println("----starting print----");
        // strBuffer.printActiveQueue();
        // end_critical();
        // Serial.println("Hanging in activity?");
        if (strBuffer.receive(p, 1000, xGetTaskId(), !shitlEnabled) > 0)
        {
            p = lineBuffer + strlen(lineBuffer);

            p[0] = '\n';
            p++;
            p[0] = '\0';

            if (p - lineBuffer > 8999 || xTaskGetTickCount() > timeout)
            { //we need to write!
                //Step 2: Write to USB
                writeUSB(lineBuffer);

                //Step 3: Write to SD card
                if (loggingEnabled)
                {
                    writeSD(lineBuffer, xGetTaskId());
                }

                //reset buffer
                lineBuffer[0] = '\0';
                p = lineBuffer;
                timeout = xTaskGetTickCount() + 1000; //if there are no logs for a bit, we should still flush every once and a while
            }
        }
        else
        { //if timeout is NEVER, we don't ever reach this (no SHITL)
            vTaskDelayUntil(&shitltimer, 10);
            readSHITL(xGetTaskId());
        }
    }
}

void LoggerTask::readSHITL(uint32_t taskid)
{
    digitalWrite(SENSOR_LED, true);
    //read in next line
    if (!f_eof(&shitl_file_object))
    {
        f_gets(inputLineBuffer, sizeof(inputLineBuffer), &shitl_file_object);

        StaticJsonDocument<1024> sensor_json;
        SensorData data;

        if (deserializeJson(sensor_json, inputLineBuffer) == DeserializationError::Ok)
        {

            JsonVariant tick = sensor_json["tick"];
            JsonVariant id = sensor_json["id"];

            if (tick.isNull() || id.isNull())
            {
                // printf("frame was invalid\n");
            }
            else
            {
                if (strcmp(id, "sensor") == 0)
                {
                    JsonVariant adxl_a_2 = sensor_json["adxl"]["a"][2];
                    JsonVariant bmp_p = sensor_json["bmp"]["p"];
                    if (adxl_a_2.isNull() || bmp_p.isNull())
                    {
                        // printf("sensor frame had invalid data\n");
                    }
                    else
                    {
                        data.tick = tick;
                        data.adxl1_data.y = adxl_a_2;
                        data.pres1_data.pressure = bmp_p;
                        data.pres2_data.pressure = bmp_p;
                        
                        sys.tasks.filter.queueSensorData(data, taskid);
                    }
                }
            }
        }
        else
        {
            // printf("Parsing Error!\n");
        }
    }
    digitalWrite(SENSOR_LED, false);
}

void LoggerTask::writeUSB(char *buf)
{
    // begin_critical();
    // Serial.println("LOGGER PRINT:---------");
    Serial.println(buf);
    // end_critical();
}

void LoggerTask::writeSD(char *buf, uint32_t taskid)
{
    digitalWrite(DISK_LED, true);

    FRESULT res;
    UINT writen;
    res = f_write(&file_object, buf, strlen(buf), &writen);

    if (res != FR_OK)
    {
        sys.tasks.logger.log("SD Write Error", taskid);
    }

    res = f_sync(&file_object); //update file structure
    if (res != FR_OK)
    {
        sys.tasks.logger.log("SD Flush Error", taskid);
    }

    digitalWrite(DISK_LED, false);
}