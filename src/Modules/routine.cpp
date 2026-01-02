#include "Modules/routine.hpp"

static const char *TAG = "routine";

Routine::~Routine() {
    for (auto event : events) {
        delete event;
    }
}

Routine::Routine(const JsonDocument &doc) : msBeforeNext(0), routineHandle(nullptr) {
    ESP_LOGD(TAG, "Deserialized config, parsing triggers");
    populateEvents(doc);
}

void                Routine::populateEvents(const JsonDocument &doc) {
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
    std::sort(events.begin(), events.end());
    ESP_LOGD(TAG, "Created %d event%s", events.size(), events.size() > 1 ? "s." : ".");
}

int                 Routine::update() {
    int     msSinceMidnight = getMsSinceMidnight();
    int     trigger = 0;
    ESP_LOGD(TAG, "%d ms since midnight (%d minutes)", msSinceMidnight, msSinceMidnight / 60000);

    for (auto event : events) {
        trigger = event->trigger(msSinceMidnight);
        if (trigger > JUST_DONE) {
            msBeforeNext = trigger;
            break;
        }
    }
    if (trigger <= 0) {
        // last event on the list either just triggered (== JUST_DONE // 0)
        // or triggered a long time ago (== PASSED // -1)
        // loop exited so no more triggers available
        // sleep until reset
        ESP_LOGI(TAG, "No more events available, resetting schedule and sleeping until 00h01");
        reset();
        return (MAX_DELAY_MS - msSinceMidnight) + 60000; // adding 1min to be sure midnight IS passed
    }
    ESP_LOGI(TAG, "%d minutes before next trigger", msBeforeNext / 60000);
    return msBeforeNext;
}

void                Routine::notify(const OutputEvent &event, uint16_t value) {
    for (auto obs : observers) {
        obs->notify(event, value);
    }
}


void                Routine::reset() {
    for (auto event : events) {
        event->reset();
    }
}

Event               *Routine::nextEvent(const std::string &type) {
    for (auto event : events) {
        if (event->wasTriggered()) {
            continue;
        }
        if (type == event->getType() || type == "any") {
            return event;
        }
    }
    return nullptr;
}

void                Routine::run() {
    xTaskCreate(routineTask, "RoutineTask", 4096, this, 1, &routineHandle);
    ESP_LOGI(TAG, "Routine task created");
}

const std::string   Routine::getStatus() const {
    if (routineHandle) {
        return "ok";
    }

    return "Routine Task not running";
}

void                Routine::routineTask(void* param) {
    Routine* routine = static_cast<Routine*>(param);
    while (true) {
        ESP_LOGI(TAG, "Running routine periodic update");
        ESP_LOGI("HEAP", "Free heap: %lu", esp_get_free_heap_size());
        int nextDelay = routine->update();

        if (nextDelay < 100) { //set min delay to 100ms
            nextDelay = 100;
        }
        ESP_LOGI(TAG, "Waiting %dms for next event", nextDelay);
        vTaskDelay(pdMS_TO_TICKS(nextDelay));
    }
    ESP_LOGE(TAG, "Exited main routine task!");
    delete routine;
}

void                Routine::updateSchedule(const JsonDocument &doc) {
    bool wasRunning = false;
    if (routineHandle) {
        wasRunning = true;
        vTaskDelete(routineHandle);
    }

    for (auto event : events) {
        delete event;
    }
    events.clear();

    populateEvents(doc);
    if (wasRunning) {
        run();
    }
}

void                Routine::getSchedule(JsonDocument &doc) const {
    JsonArray root = doc["events"].to<JsonArray>();

    for (auto evt : events) {
        JsonObject jsonEvent = root.add<JsonObject>();
        jsonEvent["type"] = evt->getType();
        jsonEvent["param"] = evt->getValue();
        int msTrigger = evt->getTriggerTimeMs();
        int hour = msTrigger / 3600000;  //ms to hour
        jsonEvent["hour"] = hour;
        jsonEvent["minutes"] = (msTrigger - hour * 3600000) / 60000;
    }
}
