#pragma once

// #include "driver/ledc.h"
#include "driver/gpio.h"

#include "Outputs/output.hpp"
#include "helpers.hpp"

class LedOutput : public Output {
    public:
                            LedOutput(JsonObjectConst);
        void                activate(int value) override;
        const std::string   getStatus() const override;


    private:
        static bool isRegistered;
        bool        on;
        uint32_t    mask;
        gpio_num_t  gpio;
};
