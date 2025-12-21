#pragma once

#include <vector>
#include <string>
#include <time.h>

#include "esp_log.h"

#define STR(x) std::string(x)
#define CSTR(x) STR(x).c_str()
#define M_ENUM_STR(x) STR(magic_enum::enum_name(x))
#define M_ENUM_CSTR(x) CSTR(magic_enum::enum_name(x))

#define LOW     0
#define HIGH    1

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

#define WEB_LOG_SIZE    250

int         pow(int x, unsigned int p);
std::string getLogs();
void        logToWebInit();
int         getMinutesSinceMidnight();
std::string readFile(const char *path);
