#include "Events/playSoundEvent.hpp"

#define TYPE "playSound"

static const char *TAG = "soundEvent";

PlaySoundEvent::PlaySoundEvent(JsonObjectConst obj) : Event(obj, TYPE) {}

void PlaySoundEvent::doStuff() {
    //play sound
}

bool PlaySoundEvent::isRegistered = EventFactory::registerType(TYPE, [](JsonObjectConst obj) {
    return new PlaySoundEvent(obj);
});

void PlaySoundEvent::overrideParam(int value, bool oneTime) {
    if (oneTime) {
        overrideValue = value;
    } else {
    }
}

int     PlaySoundEvent::getValue() const {
    // TODO
    return 1;
}

#undef TYPE
