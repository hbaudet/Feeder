#include "helpers.hpp"

static const char *TAG = "helpers";

std::string readFile(const char *path)
{
    ESP_LOGD(TAG, "reading file %s", path);
    FILE *f = fopen(path, "r");
    if (!f){
        ESP_LOGE(TAG, "Error opening file");
        return "";
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    std::string buf(size, '\0');
    fread(buf.data(), 1, size, f);
    fclose(f);
    return buf;
}

int pow(int x, unsigned int p)
{
    if (p == 0) {
        return 1;
    }
    if (p == 1) {
        return x;
    }

    int tmp = pow(x, p/2);
    if (p % 2 == 0) {
        return tmp * tmp;
    }
    return x * tmp * tmp;
}

static std::vector<std::array<char, 256>>   logBuffer;
static uint16_t                             bufferHead;

static int dual_log_printer(const char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    
    vprintf(fmt, args);

    std::array<char, WEB_LOG_SIZE> msg;
    vsnprintf(msg.data(), WEB_LOG_SIZE, fmt, args_copy);

    if (logBuffer.size() < WEB_LOG_BUFFER_SIZE) {
        logBuffer.push_back(msg);
    } else {
        logBuffer[bufferHead] = msg;
        bufferHead = (bufferHead + 1) % WEB_LOG_BUFFER_SIZE;
    }

    va_end(args_copy);
    return 0;
}

std::string getLogs() {
    std::string all("<pre>");

    if (logBuffer.size() < WEB_LOG_BUFFER_SIZE) {
        // not full => fill from 0 to size
        for (auto &line : logBuffer) {
            all += "<div>";
            all += line.data();
            all += "</div>\n";
        }
    } else {
        // full => fill from (bufferHead + 1) to buffer head (loop over end)
        for (int i = 0; i < WEB_LOG_BUFFER_SIZE; i++) {
            all += "<div>";
            all += logBuffer[(bufferHead + 1 + i) % WEB_LOG_BUFFER_SIZE].data();
            all += "</div>\n";
        }
    }
    all += "</pre>";
    return all;
}

void logToWebInit()
{
    logBuffer.reserve(WEB_LOG_SIZE);
    bufferHead = 0;
    esp_log_set_vprintf(dual_log_printer);
}

int getMinutesSinceMidnight()
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    return timeinfo.tm_hour * 60 + timeinfo.tm_min;
}

uint32_t getMsSinceMidnight()
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    return ((timeinfo.tm_hour * 60 + timeinfo.tm_min) * 60 + timeinfo.tm_sec) * 1000;
}
