#include "Modules/hmi.hpp"

static const char *TAG = "hmi";

HMI::HMI(const JsonDocument &json, Routine *routine) : lock(true), running(false), pgmValue(0),
        mode(ProgramMode(RESET)), routine(routine), blink(false) {
    uint64_t bit_mask = 0;

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE; // rising & falling edges
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    for (JsonVariantConst obj : json["inputs"].as<JsonArrayConst>()) {
        int pin = obj["gpio"];
        bit_mask |= (1ULL << pin);
        buttons[pin] = new ButtonState(obj, &lock);
        ESP_LOGD(TAG, "Created %s on pin %d", obj["type"].as<std::string>().c_str(), pin);
    }
    io_conf.pin_bit_mask = bit_mask;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_install_isr_service(0);

    for (auto btn : buttons) {
        ESP_LOGD(TAG, "adding handler for pin %d", btn.first);
        gpio_isr_handler_add((gpio_num_t)btn.first, gpioISRInterrupt, (void*)btn.first);
    }
    blinkTex = xSemaphoreCreateBinary();
}

void HMI::run() {
    buttonEventQueue = xQueueCreate(25, sizeof(ButtonEvent));
    for (auto &btn : buttons) {
        btn.second->setCallBack(buttonEventQueue);
    }
    running = true;
    xTaskCreate(HMI::hardwareListenerTask, "HardwareTask", 4096, this, 1, &hardwareTask);
    xTaskCreatePinnedToCore(HMI::buttonListenerTask, "ButtonTask", 4096, this, 1, &buttonTask, 1);
    xTaskCreatePinnedToCore(HMI::ledBlinkerTask, "BlinkTask", 4096, this, 1, &blinkTask, 1);
}

void    HMI::toggleLock(ButtonEventType type) {
    if (type != ButtonEventType::LongPressed) {
        return;
    }
    setLock(!lock);

    setPgmValue(lock ? RESET : 1);
    setMode(ProgramMode(RESET));
}

void    HMI::action(ButtonEventType type) {
    if (type != ButtonEventType::Released) {
        return;
    }
    Event * evt;
    if (mode == ProgramMode::OneTime) {
        JsonDocument json;
        std::string str;

        str = "{\"type\":\"feeder\",\"hour\":23,\"minute\":59,\"doses\":" + std::to_string(pgmValue) + "}";
        deserializeJson(json, str);
        evt = EventFactory::create(json.as<JsonObjectConst>());
        evt->addSubscriber(routine, "Subscribing to single use event");
    } else {
        evt = routine->nextEvent("feeder");
    }

    if (!evt) {
        return;
    }
    if (mode == ProgramMode::ReplaceNext) {
        evt->overrideParam(pgmValue, true);
    }
    evt->trigger(MAX_DELAY);
    if (mode == ProgramMode::OneTime) {
        delete evt;
    }
    setMode(ProgramMode(RESET));
    setPgmValue(1);
}

void    HMI::valueChange(ButtonEventType type) {
    if (type == ButtonEventType::Pressed) {
        setPgmValue((pgmValue + 1) % (PGM_MAX_VALUE + 1));
    } else if (type == ButtonEventType::LongReleased) {
        setPgmValue(1);
    }
}

void    HMI::modeChange(ButtonEventType type) {
    if (type == ButtonEventType::Released) {
        setMode(ProgramMode((mode + 1) % 2));
    } else if (type == ButtonEventType::LongReleased) {
        setMode(ProgramMode::NextAsIs);
    }
}

void    HMI::motorFeedBack(ButtonEventType type) {
    if (type == ButtonEventType::Pressed) {
        for (auto obs : observers) {
            obs->notify(OutputEvent::feederOut, 1 << FEEDER_FEEDBACK_BIT | 1);
        }
    }
}

void    HMI::setLock(bool lockHMI) {
    lock = lockHMI;
    ESP_LOGD(TAG, "Interface is now %s", lock ? "locked" : "unlocked");
    if (lock) {
        ESP_LOGD(TAG, "Turning HMI off");
        xSemaphoreTake(blinkTex, portMAX_DELAY);
    } else {
        xSemaphoreGive(blinkTex);
    }
    stateChanged();
}

void    HMI::setMode(ProgramMode pgmMode) {
    if (pgmMode == ProgramMode::NextAsIs && !routine->nextEvent("feeder")) {
        ESP_LOGD(TAG, "No next event, mode unchanged");
        return;
    }
    mode = pgmMode;
    ESP_LOGD(TAG, "Program mode set to %s", M_ENUM_CSTR(mode));
    stateChanged();
}

void    HMI::setPgmValue(uint8_t value) {
    pgmValue = value;
    ESP_LOGD(TAG, "Program value set to %d", pgmValue);
    stateChanged();
}

void    HMI::stateChanged() const {
    int value = 0;

    value |= !lock << HMI_LOCK_BIT;
    value |= pgmValue << HMI_VALUE_BIT_START;
    value |= mode << HMI_MODE_BIT;

    for (auto obs : observers) {
        obs->notify(OutputEvent::hmiLed, value);
    }
}

void    HMI::hardwareListenerTask(void *arg) {
    HMI         *caller = static_cast<HMI *>(arg);
    HmiEvent    evt;
    ESP_LOGD(TAG, "Hardware listener running");
    while (caller->running) {
        if (xQueueReceive(caller->hardwareEventQueue, &evt, portMAX_DELAY)) {
            auto err = caller->buttons[evt.pin]->trigger(evt.isPressed);
            ESP_LOGD(TAG, "Trigger return : %u", err);
            // TODO : HANDLE/LOG error
        }
        UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGD(TAG, "Stack free : %u words\n", watermark);
    }
    ESP_LOGD(TAG, "Hardware listener stopped");
    caller->running = false;
    delete caller;
}

void    HMI::buttonListenerTask(void *arg) {
    ESP_LOGD(TAG, "Created button listening task");
    HMI         *caller = static_cast<HMI *>(arg);
    ButtonEvent btn;
    while (caller->running) {
        ESP_LOGD(TAG, "Waiting for button event");
        if (xQueueReceive(caller->buttonEventQueue, &btn, portMAX_DELAY)) {
            ESP_LOGD(TAG, "Button %s is %s", CSTR(magic_enum::enum_name(btn.name)), CSTR(magic_enum::enum_name(btn.type)));
            switch (btn.name)
            {
                case ButtonName::MasterLock:
                    caller->toggleLock(btn.type);
                    break;
                case ButtonName::Mode:
                    caller->modeChange(btn.type);
                    break;
                case ButtonName::Action:
                    caller->action(btn.type);
                    break;
                case ButtonName::ValueChange:
                    caller->valueChange(btn.type);
                    break;
                case ButtonName::MotorTrigger:
                    caller->motorFeedBack(btn.type);
                    break;
                default:
                    break;
            }
        }
        UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGD(TAG, "Stack free : %u words\n", watermark);
    }
    ESP_LOGD(TAG, "Button listener stopped");
    caller->running = false;
    delete caller;
}

void    HMI::ledBlinkerTask(void *arg) {
    HMI         *caller = static_cast<HMI *>(arg);
    TickType_t  lastBlink = xTaskGetTickCount();
    Event       *next;
    while (caller->running) {
        if (xSemaphoreTake(caller->blinkTex, pdMS_TO_TICKS(WATCHDOG_TIMEOUT_MS)) == pdTRUE) {
            if (!caller->lock) {
                if (caller->mode == ProgramMode::NextAsIs) {
                    next = caller->routine->nextEvent("feeder");
                    caller->setPgmValue(!next || caller->blink ? 0 : next->getValue());
                    caller->blink = !caller->blink;
                }
            }
            xSemaphoreGive(caller->blinkTex);
        }
        xTaskDelayUntil(&(lastBlink), pdMS_TO_TICKS(BLINK_PERIOD_MS));

        UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGD(TAG, "Stack free : %u words\n", watermark);
    }
    ESP_LOGD(TAG, "LED blinker stopped");
    caller->running = false;
    delete caller;
}

QueueHandle_t HMI::hardwareEventQueue = xQueueCreate(25, sizeof(HmiEvent));

void     IRAM_ATTR HMI::gpioISRInterrupt(void *arg) {
    int pin = (int)arg;
    bool pressed = !gpio_get_level((gpio_num_t)pin);
    HmiEvent evt { pin, pressed };
    xQueueSendFromISR(hardwareEventQueue, &evt, nullptr);
}

