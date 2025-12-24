#include "Events/eventFactory.hpp"

static const char *TAG = "eventFactory";

EventFactory::EventFactory() {}

EventFactory &EventFactory::instance() {
    static EventFactory instance;
    return instance;
}

Event *EventFactory::create(JsonObjectConst obj) {
    std::string type = obj["type"];
    auto &instance = EventFactory::instance();

    auto iter = instance.makers.find(type);

    if (iter != instance.makers.end()) {
        ESP_LOGD(TAG, "Creating %s event", type.c_str());
        return iter->second(obj);
    }
    ESP_LOGE(TAG, "Couldn't retrieve %s event ctor in factory", type.c_str());

    return nullptr;
}

Event *EventFactory::createOneShot(const std::string &type, int value) {
    JsonDocument json;
    std::string jsonStr = "{\"type\":\"" + type + "\",\"hour\":23,\"minute\":59,\"param\":" + std::to_string(value) + "}";
    deserializeJson(json, jsonStr);

    return create(json.as<JsonObject>());
}

bool EventFactory::registerType(const std::string &type, std::function<Event*(JsonObjectConst)> func) {
    auto &instance = EventFactory::instance();
    instance.makers[type] = func;

    ESP_LOGD(TAG, "registered %s event ctor in factory", type);
    return true;
}
