#pragma once

#include <ArduinoJson.h>
#include <unordered_map>
#include <string>

#include "esp_log.h"
#include "Outputs/output.hpp"
#include "Outputs/outputFactory.hpp"
#include "IObserver.hpp"
#include "magic_enum/magic_enum.hpp"
#include "debug.hpp"
#include "helpers.hpp"

class OutputManager : public IObserver<OutputEvent> {
    public:
                ~OutputManager();
                OutputManager() = delete;
                OutputManager(const JsonDocument &);
        void    notify(const OutputEvent &, uint16_t) override;

    private:
        std::unordered_map<std::string, Output*>               outputs;
        std::unordered_map<std::string, std::vector<Output*>>  ioMatrix;
};
