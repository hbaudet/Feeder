#include "main.hpp"

#ifndef UNIT_TEST
static const char *TAG = "main";

extern "C" int app_main(void)
{
    Routine         *feederRoutine;
    HMI             *hmi;
    OutputManager   *outManager;
    std::string     config;
    JsonDocument    doc;

    initStorage();
    initLocalSetup();
    ESP_LOGD(TAG, "local setup done, reading config json");
    config = readFile("/spiffs/configuration.json");
    auto err = deserializeJson(doc, config);

    if (err != DeserializationError::Code::Ok) {
        ESP_LOGE(TAG, "Unable to deserialize json config file [%s]", magic_enum::enum_name(err.code()));
        ESP_LOGD(TAG, "faulty json :\n%s\n", config.c_str());
        return -1;
    }

    // Create Logic Modules
    outManager = new OutputManager(doc);
    feederRoutine = new Routine(doc);
    hmi = new HMI(doc, feederRoutine);
    assert(feederRoutine->addSubscriber(outManager, "Outputmanager subscribing to feeder routine") == true);
    assert(hmi->addSubscriber(outManager, "Outputmanager subscribing to hmi") == true);
    WebServ::setHooks(feederRoutine, outManager, hmi);
    hmi->run();
    feederRoutine->run();
    WebServ::run();

    return 0;
}
#endif
