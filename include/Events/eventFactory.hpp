#pragma once

#include <ArduinoJson.h>
#include <esp_log.h>

#include <map>
#include <functional>
#include <memory>

class Event;

class EventFactory {
    public:
        static Event        *create(JsonObjectConst);
        static bool         registerType(const std::string&, std::function<Event*(JsonObjectConst)>);

        EventFactory(const EventFactory&) = delete;
        EventFactory &operator=(const EventFactory&) = delete;

    private:
        static EventFactory &instance();

        EventFactory();
        std::map<std::string, std::function<Event*(JsonObjectConst)>> makers;
};
