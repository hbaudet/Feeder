#include "Events/feederEvent.hpp"

#define TYPE "feeder"

static const char *TAG = "feederEvent";

FeederEvent::FeederEvent(JsonObjectConst obj) : Event(obj, TYPE) {
    doses = obj["param"];
}

void    FeederEvent::doStuff() {
    int value;

    if (overrideValue == -1) {
        value = doses;
    } else {
        value = overrideValue;
    }
    ESP_LOGI(TAG, "Feeding %d doses", value);
    for (auto obs : observers) {
        obs->notify(OutputEvent::feederOut, 1 << FEEDER_COUNT_BIT | value);
    }
    overrideValue = -1;
}

void    FeederEvent::overrideParam(int value, bool oneTime) {
    if (oneTime) {
        overrideValue = value;
    } else {
        doses = value;
    }
}

int     FeederEvent::getValue() const {
    return doses;
}

bool    FeederEvent::isRegistered = EventFactory::registerType(TYPE, [](JsonObjectConst obj) {
    return new FeederEvent(obj);
});

#undef TYPE
