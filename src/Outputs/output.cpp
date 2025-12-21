#include "Outputs/output.hpp"

static const char *TAG = "output";

Output::Output(JsonObjectConst obj, const std::string &type) : type(type) {
    assert(obj["name"].is<const char*>());
    name = std::string(obj["name"].as<const char *>());
}

const std::string   &Output::getName() const {
    return name;
}
