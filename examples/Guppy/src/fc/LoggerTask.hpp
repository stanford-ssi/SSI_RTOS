#pragma once

#include <FreeRTOS.h>
#include "MsgBuffer.hpp"
#include <string.h>
#include "ArduinoJson.h"
#include "SSISD.hpp"
#include "StrBuffer.hpp"
#include "Task.hpp"

#define DISK_LED 3

enum log_type
{
  fatal = 1,
  error = 2,
  warning = 4,
  stats = 8,
  data = 16,
  info = 32
};

class LoggerTask : public Task<1000>
{
private:
  static StrBuffer<10000> strBuffer;

  static char lineBuffer[10000];

  static char inputLineBuffer[1000];

  static FATFS fs;
  static FIL file_object;
  static FIL shitl_file_object;

  static bool loggingEnabled;
  static bool shitlEnabled;

  void activity();
  static void readSHITL(uint32_t taskid);
  static void writeUSB(char *buf);
  static void writeSD(char *buf, uint32_t taskid);
  static void format();

public:
  LoggerTask(uint8_t priority);
  void log(const char *message, uint32_t taskid);
  void log(JsonDocument &jsonDoc, uint32_t taskid);
  void logJSON(JsonDocument &jsonDoc, const char *id, uint32_t taskid);
  bool isLoggingEnabled() { return loggingEnabled; };
};