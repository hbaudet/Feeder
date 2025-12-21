#include "Modules/outputManager.hpp"

static const char *TAG = "outputManager";

OutputManager::~OutputManager() {
    for (auto out : outputs) {
        delete out.second;
    }
}

OutputManager::OutputManager(const JsonDocument &json) {
    for (JsonObjectConst obj : json["outputs"].as<JsonArrayConst>()) {
        Output  *out = OutputFactory::create(obj);
        if (!out) {
            continue;
        }
        if (out == nullptr) {
            ESP_LOGD(TAG, "THIS is the probleme: %p", out);
        }
        ESP_LOGD(TAG, "Emplacing %p into outputs as %s", out, CSTR(obj["name"].as<const char*>()));
        assert(outputs.emplace(std::string(obj["name"].as<const char*>()), out).second);
    }
    for (JsonPairConst kv : json["ioMatrix"].as<JsonObjectConst>()) {
        ESP_LOGD(TAG, "Linking %s to its outputs", kv.key().c_str());
        for (std::string output : kv.value().as<JsonArrayConst>()) {
            if (!outputs.contains(output)) {
                continue;
            }
            ESP_LOGD(TAG, "Pushing %p(%s) into %s vector", outputs[output], output.c_str(), kv.key().c_str());
            ioMatrix[STR(kv.key().c_str())].push_back(outputs[output]);
        }
    }
}

void    OutputManager::notify(const OutputEvent &event, uint16_t value) {
    ESP_LOGD(TAG, "Output manager notified to perform %s at %d", M_ENUM_CSTR(event), value);
    for (Output *out : ioMatrix[M_ENUM_STR(event)]) {
        out->activate(value);
    }
}
