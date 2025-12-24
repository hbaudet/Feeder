#include "Outputs/motorOutput.hpp"

#define TYPE "motor"

static const char *TAG = "motorOutput";

MotorOutput::MotorOutput(JsonObjectConst json) : Output(json, TYPE), count(0), reverse(false), running(false) {
    ESP_LOGD(TAG, "Creating motor, speed resolution : %d, max speed : %d, max value: %d",
                SPEED_RESOLUTION, MAX_SPEED, (pow(2, PGM_VALUE_RESOLUTION) - 1));
    speed = json["speed"];
    setSpeed(speed);
    directionPin[0] = json["gpio"][0];
    directionPin[1] = json["gpio"][1];

    gpio_config_t directionConf = {
        .pin_bit_mask = (1ULL << directionPin[0] | (1ULL << directionPin[1])),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&directionConf));

    ledc_timer_config_t pwmTimer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = SPEED_RESOLUTION,
        .timer_num = LEDC_TIMER,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&pwmTimer));

    ledc_channel_config_t pwmChannel = {
        .gpio_num   = json["gpio"][2],
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&pwmChannel));
}

void                MotorOutput::setSpeed(int desiredSpeed) {
    if (desiredSpeed > MAX_SPEED) {
        desiredSpeed = MAX_SPEED;
    } else if (desiredSpeed < 0) {
        desiredSpeed = 0;
    }

    if (running) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, desiredSpeed));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL));
    }
}

void                MotorOutput::activate(int value) {
    ESP_LOGD(TAG, "Activating feeder motor");
    ESP_LOGD(TAG, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(value));
    if (value & 1 << FEEDER_COUNT_BIT) {
        count += value & PGM_MAX_VALUE;
    } else if (value & 1 << FEEDER_FEEDBACK_BIT) {
        count -= value & PGM_MAX_VALUE;
    }
    ESP_LOGD(TAG, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(count));

    if (count <= 0) {
        stop();
        return;
    }
    ESP_LOGD(TAG, "Feeder Motor running, %d cycles left", count);
    running = true;
    setSpeed(speed);

    ESP_ERROR_CHECK(gpio_set_level(directionPin[0], reverse ? 0 : 1));
    ESP_ERROR_CHECK(gpio_set_level(directionPin[1], reverse ? 1 : 0));
}

void                MotorOutput::setDirection(bool reverse) {
    this->reverse = reverse;
}

void                MotorOutput::stop() {
    ESP_LOGD(TAG, "Stopping feeder motor");
    setSpeed(0);
    running = false;
    count = 0;

    ESP_ERROR_CHECK(gpio_set_level(directionPin[0], 0));
    ESP_ERROR_CHECK(gpio_set_level(directionPin[1], 0));
}

const std::string   MotorOutput::getStatus() const {
    if (running) {
        return "Feeder motor running : " + std::to_string(count) + " doses remaining";
    }
    return "Feeder motor stopped";
}

bool    MotorOutput::isRegistered = OutputFactory::registerType(TYPE, [](JsonObjectConst obj) {
    return new MotorOutput(obj);
});

#undef TYPE
