#include "Outputs/ledOutput.hpp"

#define TYPE "led"

static const char *TAG = "ledOutput";

LedOutput::LedOutput(JsonObjectConst obj) : Output(obj, TYPE), on(false) {
    gpio = obj["gpio"];
    mask = obj["mask"];
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio, LOW);
}

void        LedOutput::activate(int value) {
    on = value & mask;
    gpio_set_level(gpio, on);
}

bool    LedOutput::isRegistered = OutputFactory::registerType(TYPE, [](JsonObjectConst obj) {
    return new LedOutput(obj);
});

#undef TYPE
