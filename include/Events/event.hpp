#pragma once

#include <ArduinoJson.h>
#include <string>
#include <esp_log.h>

#include "Events/eventFactory.hpp"
#include "IObserver.hpp"
#include "Outputs/output.hpp"
#include "debug.hpp"

#define PASSED              -1
#define JUST_DONE           0

#define IGNORE_EVENT_DELAY_MS   300000      // (5 minutes)

#define MAX_DELAY               1440        // minutes in a day
#define MAX_DELAY_MS            86400000    // ms in a day

class Event : public Observable<OutputEvent> {
    public:
                            Event(JsonObjectConst obj, const std::string &type);
        virtual             ~Event();
        void                reset();
        int                 trigger(uint32_t);
        virtual void        overrideParam(int value, bool oneTime) = 0;
        bool                wasTriggered() const;
        const std::string   &getType() const;
        int                 getTriggerTimeMs() const;
        virtual int         getValue() const = 0;

    protected:
        virtual void        doStuff() = 0;
        int                 overrideValue;

    private:
        std::string         type;
        int                 hour;
        int                 minute;
        int                 triggerDelayMs;
        bool                triggeredToday;
};
