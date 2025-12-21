#include "Modules/routine.hpp"

static const char *TAG = "routine";

Routine::~Routine() {
    for (auto event : events) {
        delete event;
    }
}

Routine::Routine(const JsonDocument &doc) : msBeforeNext(0) {
    ESP_LOGD(TAG, "Deserialized config, parsing triggers");
    JsonArrayConst arr = doc["triggers"].as<JsonArrayConst>();
    for (JsonObjectConst obj : arr) {
        Event *t = EventFactory::create(obj);
        if (!t) {
            ESP_LOGE(TAG, "Couldn't create event, skipping");
            continue;
        }
        events.push_back(t);
        t->addSubscriber(this, std::string("Feeder Routine subscribing to ").append(t->getType().c_str()));
    }
    ESP_LOGD(TAG, "Created %d event%s", events.size(), events.size() > 1 ? "s." : ".");
}

int     Routine::update() {
    int     minutesSinceMidnight = getMinutesSinceMidnight();
    int     minBeforeNext = 0;
    int     trigger = 0;
    ESP_LOGD(TAG, "%d minutes since midnight", minutesSinceMidnight);

    for (auto event : events) {
        trigger = event->trigger(minutesSinceMidnight);
        if (trigger > 0) {
            minBeforeNext = trigger;
            msBeforeNext = minBeforeNext * 60000;
            break;
        }
    }
    if (trigger <= 0) {
        // last event on the list either just triggered
        // or triggered a long time ago
        // loop exited so no more triggers available
        // sleep until reset
        ESP_LOGD(TAG, "No more events available!");
        return (MAX_DELAY - minutesSinceMidnight) * 60000;
    }
    ESP_LOGI(TAG, "%d minutes before next trigger", minBeforeNext);
    return msBeforeNext;
}

void    Routine::notify(const OutputEvent &event, uint16_t value) {
    for (auto obs : observers) {
        obs->notify(event, value);
    }
}


void    Routine::reset() {
    for (auto event : events) {
        event->reset();
    }
}

Event   *Routine::nextEvent(const std::string &type) {
    for (auto event : events) {
        if (event->wasTriggered()) {
            continue;
        }
        if (type == event->getType()) {
            return event;
        }
    }
    return nullptr;
}

void    Routine::run() {
    xTaskCreate(routineTask, "RoutineTask", 4096, this, 1, nullptr);
    ESP_LOGD(TAG, "Routine task created");
}

void    Routine::routineTask(void* param) {
    Routine* routine = static_cast<Routine*>(param);
    while (true) {
        ESP_LOGD(TAG, "Running routine periodic update");
        int nextDelay = routine->update();

        if (nextDelay < 100) { //set min delay to 100ms
            nextDelay = 100;
        }
        vTaskDelay(pdMS_TO_TICKS(nextDelay));
    }
    ESP_LOGE(TAG, "Exited main routine task!");
    delete routine;
}
