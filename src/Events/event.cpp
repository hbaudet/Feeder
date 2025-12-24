#include "Events/event.hpp"

static const char *TAG = "event";

Event::~Event() {
    observers.clear();
}

Event::Event(JsonObjectConst obj, const std::string &type) : overrideValue(-1), type(type), triggeredToday(false) {
    hour   = obj["hour"];
    minute = obj["minute"];

    triggerDelayMs = (hour * 60 + minute) * 60000;
}

int            Event::trigger(uint32_t msSinceMidnight) {
    if (triggeredToday) {
        ESP_LOGD(TAG, "Event already triggered, passing");
        return PASSED;
    }

    if (msSinceMidnight < triggerDelayMs) {
        ESP_LOGD(TAG, "Event yet to come (%02dH%02d)", hour, minute);
        return triggerDelayMs - msSinceMidnight;
    }

    triggeredToday = true;
    if (msSinceMidnight > triggerDelayMs + IGNORE_EVENT_DELAY_MS) {
        // assume it was done before boot since trigger time was
        // more than X ms before boot
        // => do nothing and pretend its done
        return JUST_DONE;
    }

    doStuff();

    return JUST_DONE;
}

bool                Event::wasTriggered() const {
    return triggeredToday;
}

const std::string   &Event::getType() const {
    return type;
}

int                 Event::getTriggerTimeMs() const {
    return triggerDelayMs;
}

void                Event::reset() {
    triggeredToday = false;
}
