#pragma once

#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>

#include "esp_sntp.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_spiffs.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "debug.hpp"
#include "wifiSetup.hpp"
#include "helpers.hpp"

void        wifiInit();
void        sntpInit();
void        initLocalSetup();
void        initStorage();
