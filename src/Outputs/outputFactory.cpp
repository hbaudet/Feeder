#include "Outputs/outputFactory.hpp"

static const char *TAG = "OutputFactory";

OutputFactory::OutputFactory() {}

OutputFactory &OutputFactory::instance() {
    static OutputFactory instance;
    return instance;
}

Output *OutputFactory::create(JsonObjectConst obj) {
    std::string type = obj["type"];
    auto &instance = OutputFactory::instance();

    std::string output;
    serializeJson(obj, output);
    auto iter = instance.makers.find(type);

    if (iter != instance.makers.end()) {
        ESP_LOGD(TAG, "Creating %s Output", type.c_str());
        return iter->second(obj);
    }
    ESP_LOGE(TAG, "Couldn't retrieve %s Output ctor in factory", type.c_str());

    return nullptr;
}

bool OutputFactory::registerType(const std::string &type, std::function<Output*(JsonObjectConst)> func) {
    auto &instance = OutputFactory::instance();
    instance.makers[type] = func;

    ESP_LOGD(TAG, "registered %s Output ctor in factory", type);
    return true;
}
