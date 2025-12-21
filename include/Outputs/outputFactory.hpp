#pragma once

#include <ArduinoJson.h>
#include <esp_log.h>

#include <map>
#include <functional>
#include <memory>

class Output;

class OutputFactory {
    public:
        static Output       *create(JsonObjectConst);
        static bool         registerType(const std::string&, std::function<Output*(JsonObjectConst)>);

        OutputFactory(const OutputFactory&) = delete;
        OutputFactory &operator=(const OutputFactory&) = delete;

    private:
        static OutputFactory &instance();

        OutputFactory();
        std::map<std::string, std::function<Output*(JsonObjectConst)>> makers;
};
