#pragma once

#include <unordered_set>
#include <ArduinoJson.h>

#include "Outputs/outputFactory.hpp"
#include "IObserver.hpp"
#include "debug.hpp"
#include "helpers.hpp"

#define PGM_VALUE_RESOLUTION    4
#define PGM_MAX_VALUE           (pow(2, PGM_VALUE_RESOLUTION) - 1)
#define FEEDER_COUNT_BIT        PGM_VALUE_RESOLUTION
#define FEEDER_FEEDBACK_BIT     (FEEDER_COUNT_BIT + 1)
#define HMI_LOCK_BIT            0
#define HMI_VALUE_BIT_START     1
#define HMI_MODE_BIT            (HMI_VALUE_BIT_START + PGM_VALUE_RESOLUTION)
#define BLINK_PERIOD_MS         350

enum class OutputEvent {
    hmiLed,
    feederOut,
    lightOut,
    audioOut
};

class Output {
    public:
        virtual             ~Output() = default;
                            Output() = delete;
                            Output(const Output &) = delete;
                            Output(Output &&) = delete;
        Output              &operator=(const Output &) = delete;
        Output              &operator=(Output &&) = delete;
                            Output(JsonObjectConst, const std::string &type);
        const std::string   &getName() const;
        virtual void        activate(int value) = 0;

    private:
        std::string         name;
        std::string         type;
};
