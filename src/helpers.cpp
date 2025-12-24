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

static std::vector<std::string> log_buffer;  // circular buffer in RAM

static int dual_log_printer(const char *fmt, va_list args)
{
    vprintf(fmt, args);

    char msg[256];
    vsnprintf(msg, sizeof(msg), fmt, args);
    log_buffer.emplace_back(msg);

    if (log_buffer.size() > WEB_LOG_SIZE) {
        log_buffer.erase(log_buffer.begin());
    }

    return 0;
}

std::string getLogs() {
    std::string all;
    for (auto &line : log_buffer)
        all += "<p>" + line + "</p>";
    return all;
}

void logToWebInit()
{
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
