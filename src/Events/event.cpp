#include "Events/event.hpp"

static const char *TAG = "event";

Event::~Event() {
    observers.clear();
}

Event::Event(JsonObjectConst obj, const std::string &type) : overrideValue(-1), type(type), triggeredToday(false) {
    hour   = obj["hour"];
    minute = obj["minute"];

    triggerDelayMin = hour * 60 + minute;
}

int                 Event::trigger(int minutesSinceMidnight) {
    if (triggeredToday) {
        ESP_LOGD(TAG, "Event already triggered, passing");
        return PASSED;
    }

    if (minutesSinceMidnight < triggerDelayMin) {
        ESP_LOGD(TAG, "Event yet to come (%02dH%02d)", hour, minute);
        return triggerDelayMin - minutesSinceMidnight;
    }

    triggeredToday = true;
    if (minutesSinceMidnight > triggerDelayMin + IGNORE_EVENT_DELAY) {
        // assume it was done before boot since trigger time was
        // more than X minutes before boot
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

void                Event::reset() {
    triggeredToday = false;
}
