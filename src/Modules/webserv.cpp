#include "Modules/webserv.hpp"

static const char *TAG = "webserv";

WebServ::WebServ() : server(nullptr) {
    config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEBSERV_PORT;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16;
}

WebServ     &WebServ::instance() {
    static WebServ instance;
    return instance;
}

esp_err_t   WebServ::favicon(httpd_req_t *req) {
    const char *filepath = "/spiffs/favicon.png";

    FILE *file = fopen(filepath, "r");
    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/png");

    char buffer[512];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }

    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t   WebServ::getLogHandler(httpd_req_t *req) {
    // TODO : see if websocket for live logging doable

    ESP_LOGI("HEAP/LOG", "Free heap: %lu", esp_get_free_heap_size());
    httpd_resp_set_type(req, "text/html");
    std::string logs = getLogs();
    httpd_resp_send(req, logs.c_str(), logs.size());
    return ESP_OK;
}

esp_err_t   WebServ::getIndexHandler(httpd_req_t *req) {
    // might be deprecated if no use for it is found
    auto html = readFile("/spiffs/index.html");
    if (html.empty()) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.size());
    return ESP_OK;
}

esp_err_t   WebServ::rebootHandler(httpd_req_t *req) {
    std::string ret = "Rebooting in 2 seconds";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, ret.c_str(), ret.size());

    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    return ESP_OK;
}

esp_err_t   WebServ::lockHandler(httpd_req_t *req) {
    std::string ret = "HMI locked";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, ret.c_str(), ret.size());
    instance().hmi->setLock(true);
    return ESP_OK;
}

esp_err_t   WebServ::unlockHandler(httpd_req_t *req) {
    std::string ret = "HMI unlocked";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, ret.c_str(), ret.size());
    instance().hmi->setLock(false);
    return ESP_OK;
}

void        WebServ::getUptimeJson(JsonDocument &doc) {
    int64_t uptime_us = esp_timer_get_time();
    int64_t uptime_sec = uptime_us / 1000000;
    int days = uptime_sec / (24 * 3600);
    int hours = (uptime_sec % (24 * 3600)) / 3600;
    int minutes = (uptime_sec % 3600) / 60;
    int seconds = uptime_sec % 60;

    JsonObject root = doc["upTime"].to<JsonObject>();
    root["days"] = days;
    root["hours"] = hours;
    root["minutes"] = minutes;
    root["seconds"] = seconds;
}

void        WebServ::getTimeJson(JsonDocument &doc) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    JsonObject root = doc["localTime"].to<JsonObject>();
    root["hour"] = timeinfo.tm_hour;
    root["minute"] = timeinfo.tm_min;
    root["second"] = timeinfo.tm_sec;
}

void        WebServ::getNextEventJson(JsonDocument &doc) {
    auto evt = instance().routine->nextEvent("any");
    
    if (!evt) {
        doc["nextEvent"] = false;
        return;
    }
    JsonObject root = doc["nextEvent"].to<JsonObject>();
    root["type"] = evt->getType();
    int triggerTime = evt->getTriggerTimeMs() / 60000;
    JsonObject time = root["triggerTime"].to<JsonObject>();
    time["hour"] = triggerTime / 60;
    time["minutes"] = triggerTime % 60;
    root["triggerDelay"] = triggerTime - getMinutesSinceMidnight();
}

void        WebServ::getModulesStatusJson(JsonDocument &doc) {
    JsonObject root = doc["status"].to<JsonObject>();
    JsonDocument outputs;

    if (instance().routine) {
        root["Scheduler"] = instance().routine->getStatus();
    }
    if (instance().output) {
        if (deserializeJson(outputs, instance().output->getStatus()) != DeserializationError::Ok) {
            root["OutputManager"] = "Unable to get output status";
        } else {
            root["OutputManager"] = outputs.as<JsonObject>();
        }
    }
    if (instance().hmi) {
        root["HMI"] = instance().hmi->getStatus();
    }
    root["Fragmentation"] = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    root["Heap"] = esp_get_free_heap_size();
    root["Stack"] = uxTaskGetStackHighWaterMark(NULL);
}

esp_err_t   WebServ::apiStatusHandler(httpd_req_t *req) {
    std::string resp;
    // TODO : get network info (whatever is available)

    JsonDocument doc;

    getUptimeJson(doc);
    getTimeJson(doc);
    getNextEventJson(doc);
    getModulesStatusJson(doc);

    serializeJson(doc, resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp.c_str(), resp.size());
    return ESP_OK;
}

esp_err_t   WebServ::apiScheduleHandler(httpd_req_t *req) {
    std::string resp;
    JsonDocument doc;

    instance().routine->getSchedule(doc);

    serializeJson(doc, resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp.c_str(), resp.size());
    return ESP_OK;
}

esp_err_t   WebServ::apiEventAddHandler(httpd_req_t *req) {
    const char *uri = req->uri;

    // Expected format: /api/add/<type>/<number>
    if (strncmp(uri, API_ADD_BASE_URI, strlen(API_ADD_BASE_URI)) != 0) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Extract the part after "/api/add/"
    const char *path = uri + strlen(API_ADD_BASE_URI);

    // Find first '/'
    char *slash = strchr(path, '/');
    if (!slash) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing value");
        return ESP_FAIL;
    }

    // Extract type (before '/')
    char type[32];
    int len = slash - path;
    if (len >= sizeof(type)) {
        len = sizeof(type) - 1;
    }
    memcpy(type, path, len);
    type[len] = '\0';

    // Extract number (after '/')
    const char *num_str = slash + 1;
    int value = atoi(num_str);

    // Validate
    if (value <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid value");
        return ESP_FAIL;
    }

    auto evt = EventFactory::createOneShot(type, value);
    evt->addSubscriber(instance().routine, "Subscribing to single use event from API");
    evt->trigger(MAX_DELAY_MS);
    delete evt;

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t   WebServ::apiUpdateScheduleHandler(httpd_req_t *req) {
    size_t content_len = req->content_len;

    if (content_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }

    // Allocate buffer for body
    char *body = (char*)malloc(content_len + 1);
    if (!body) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }

    // Read body
    int ret = httpd_req_recv(req, body, content_len);
    if (ret <= 0) {
        free(body);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read body");
        return ESP_FAIL;
    }
    body[content_len] = '\0';  // Null-terminate

    // Parse into JsonDocument (assume valid)
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    free(body);

    if (err != DeserializationError::Ok) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, err.c_str());
        return ESP_FAIL;
    }

    instance().routine->updateSchedule(doc);
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void        WebServ::setHooks(Routine *rout, OutputManager *out, HMI *hmi) {
    instance().routine = rout;
    instance().output = out;
    instance().hmi = hmi;
}

void        WebServ::registerUri(const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r)) {
    httpd_uri_t httpUri = {
            .uri = uri,
            .method = method,
            .handler = handler,
            .user_ctx = nullptr
    };

    auto err = httpd_register_uri_handler(instance().server, &httpUri);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not register %s handler", uri);
    }
}

void        WebServ::run() {
    if (httpd_start(&(instance().server), &(instance().config)) == ESP_OK)
    {
        logToWebInit();
        registerUri("/favicon.ico",         HTTP_GET,   favicon);
        registerUri("/",                    HTTP_GET,   getIndexHandler);
        registerUri("/restart",             HTTP_GET,   rebootHandler);
        registerUri("/lock",                HTTP_GET,   lockHandler);
        registerUri("/unlock",              HTTP_GET,   unlockHandler);
        registerUri("/api/status",          HTTP_GET,   apiStatusHandler);
        registerUri("/api/schedule",        HTTP_GET,   apiScheduleHandler);
        registerUri("/api/add/*",           HTTP_GET,   apiEventAddHandler);
        registerUri("/api/update/schedule", HTTP_POST,  apiUpdateScheduleHandler);
        registerUri("/logs",                HTTP_GET,   getLogHandler);

        ESP_LOGI(TAG, "HTTP server ready on port %d", instance().config.server_port);
    }
}
