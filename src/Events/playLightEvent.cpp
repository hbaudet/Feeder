#include "Events/playLightEvent.hpp"

#define TYPE "playLight"

static const char *TAG = "lightEvent";

PlayLightEvent::PlayLightEvent(JsonObjectConst obj) : Event(obj, TYPE) {}

void PlayLightEvent::doStuff() {
    //play light
}

bool PlayLightEvent::isRegistered = EventFactory::registerType(TYPE, [](JsonObjectConst obj) {
    return new PlayLightEvent(obj);
});

void PlayLightEvent::overrideParam(int value, bool oneTime) {
    if (oneTime) {
        overrideValue = value;
    } else {
    }
}

int     PlayLightEvent::getValue() const {
    // TODO
    return 1;
}

#undef TYPE
