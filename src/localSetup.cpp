#include "localSetup.hpp"

static const char* TAG = "localSetup";

void wifiInit()
{
    //RESET just in case
    esp_wifi_stop();
    esp_wifi_deinit();
    //
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASS);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();

    ESP_LOGI(TAG, "Connecting to Wi-Fi...");

    mdns_free();
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGE(TAG, "MDNS Init failed: %d\n", err);
        return;
    }
    mdns_hostname_set("onafaim");
    mdns_instance_name_set("Holy Cat Grail");

    vTaskDelay(pdMS_TO_TICKS(5000));
}

void sntpInit() {
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now = 0;
    int retry = 0;
    const int retry_count = 10;
    while (time(&now) < 1577836800 && ++retry < retry_count) { // 2020-01-01
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void wifiEvent(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    wifi_event_t wifiId = static_cast<wifi_event_t>(id);

    if (id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected, reconnecting...");
        esp_wifi_connect();
    } else {
        ESP_LOGI(TAG, "Received %s event : %s", base, M_ENUM_CSTR(wifiId));
    }
}

void initLocalSetup() {
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEvent, NULL, NULL));
    wifiInit();
    sntpInit();
    setenv("TZ", "CET-1CEST-2,M3.5.0/2,M10.5.0/3", 1); // Paris time, include DST
    tzset();

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "Current time: %02d:%02d:%02d",
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void initStorage() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unable to create NVS");
        return;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = nullptr,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "SPIFFS Error: %s", esp_err_to_name(ret));
    else
        ESP_LOGI(TAG, "SPIFFS mounted");

#ifdef CHECK_SPIFFS_STARTUP

    ESP_LOGD(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s).", esp_err_to_name(ret));
        // esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGD(TAG, "Partition size: total: %d, used: %d", total, used);
    }

#endif
}
