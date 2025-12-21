#include "Modules/buttonState.hpp"

static const char *TAG = "button";

ButtonState::ButtonState(JsonVariantConst obj, const bool *lock) :
    isPressed(false), wentDown(0), lastChange(xTaskGetTickCount()), callback(nullptr), masterLock(lock), lockable(true), stopTasks(false) {
    gpio = obj["gpio"];
    auto nameJson = magic_enum::enum_cast<ButtonName>(obj["type"].as<std::string>());
    if (nameJson.has_value()) {
        name = nameJson.value();
    } else {
        name = ButtonName::ERROR_INVALID;
    }
    if (obj["lockable"].is<bool>()) {
        lockable = obj["lockable"].as<bool>();
    }
    mutex = xSemaphoreCreateMutex();

    ESP_LOGI(TAG, "Created ButtonState %p", this);
}

ButtonState::~ButtonState() {
    stopTasks = true;

    if (isPressed) {
        // task already killed on release,
        // only kill it if button is destroyed before release
        esp_timer_stop(longPressTimer);
        esp_timer_delete(longPressTimer);
    }

    if (mutex) {
        vSemaphoreDelete(mutex);
    }
}

bool        ButtonState::isValid() const {
    return name < ButtonName::ERROR_INVALID;
}

uint8_t     ButtonState::trigger(bool pressed) {
    if (!isValid()) {
        return INVALID;
    }
    uint8_t status;
    if (lock()) {
        if (*masterLock && lockable) {
            ESP_LOGI(TAG, "%s cannot fire when HMI is locked", M_ENUM_CSTR(name));
            assert(unlock());
            return LOCKED;
        }

        if (pressed == this->isPressed) {
            //duplicate trigger that doesn't need handling
            assert(unlock());
            return DUPLICATE;
        }

        TickType_t now = xTaskGetTickCount();
        if ((now - lastChange) * portTICK_PERIOD_MS < DELAY_DEBOUNCE_MS) {
            lastChange = now;
            assert(unlock());
            return BOUNCE;
        }
        lastChange = now;

        this->isPressed = pressed;
        if (this->isPressed) { //new state : pressed
            wentDown = now;
            status = fireEvent(ButtonEventType::Pressed);
            fireLongPressTimer();
        } else { // new state : released
            status = release();
        }
        assert(unlock());
        return status;
    }
    ESP_LOGD(TAG, "%s couldn't get the semaphore, no handling done", M_ENUM_CSTR(name));
    return ACCES_DENIED;
}

void        ButtonState::fireLongPressTimer() {
    esp_timer_create_args_t args = {
        .callback = &ButtonState::timerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "longPressTimer",
        .skip_unhandled_events = false,
    };

    auto err = esp_timer_create(&args, &longPressTimer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error creating long press timer [%d]", err);
        return;
    }
    err = esp_timer_start_once(longPressTimer, DELAY_LONG_PRESS_MS * 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting long press timer [%d]", err);
    }
}

void        ButtonState::onLongPress() {
    if (stopTasks) {
        ESP_LOGE(TAG, "Long press cancelled for %s/%p, should have been intercepted earlier!", M_ENUM_CSTR(name), this);
        return;
    }
    if (lock()) {
        if (isPressed) {
            fireEvent(ButtonEventType::LongPressed);
        }
        assert(unlock());
    }
}

uint8_t     ButtonState::fireEvent(ButtonEventType type) const {
    if (!callback) {
        ESP_LOGE(TAG, "%s no callback for state change", M_ENUM_CSTR(name));
        return NO_CALLBACK;
    }

    ButtonEvent event { type, name };

    BaseType_t err = xQueueSend(callback, &event, (TickType_t)0);
    if (err != pdPASS) {
        ESP_LOGE(TAG, "Error sending -%s- button state event to queue [%d]\n", M_ENUM_CSTR(name), err);
        return FAILED;
    }
    return FIRED;
}

void        ButtonState::setCallBack(const QueueHandle_t queue) {
    callback = queue;
}

ButtonName  ButtonState::getName() const {
    return name;
}

bool        ButtonState::lock() {
#ifndef UNIT_TEST
    bool ret = xSemaphoreTake(mutex, portMAX_DELAY);
#else
    bool ret = pdTRUE;
#endif

    return ret == pdTRUE;
}

bool        ButtonState::unlock() {
#ifndef UNIT_TEST
    bool ret = xSemaphoreGive(mutex);
#else
    bool ret = pdTRUE;
#endif

    return ret == pdTRUE;
}

uint8_t     ButtonState::release() {
    ButtonEventType type;

    esp_timer_stop(longPressTimer);
    esp_timer_delete(longPressTimer);

    if ((lastChange - wentDown) * portTICK_PERIOD_MS >= DELAY_LONG_PRESS_MS) {
        type = ButtonEventType::LongReleased;
    } else {
        type = ButtonEventType::Released;
    }

    return fireEvent(type);
}

void        ButtonState::timerCallback(void* arg) {
    ButtonState*    caller = static_cast<ButtonState*>(arg);

    if (!caller->stopTasks) {
        caller->onLongPress();
    }
}
