#pragma once

#include "driver/ledc.h"
#include "driver/gpio.h"

#include "Outputs/output.hpp"
#include "helpers.hpp"

#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER LEDC_TIMER_0
#define SPEED_RESOLUTION LEDC_TIMER_10_BIT

#define MAX_SPEED (pow(2, SPEED_RESOLUTION) - 1)

class MotorOutput : public Output {
    public:
                            MotorOutput(JsonObjectConst);
        void                activate(int value) override;
        void                setDirection(bool reverse);
        void                stop();
        void                setSpeed(int speed);
        const std::string   getStatus() const override;


    private:
        static bool isRegistered;

        int         count;
        bool        reverse;
        bool        running;
        int         speed;
        gpio_num_t  directionPin[2];
};
