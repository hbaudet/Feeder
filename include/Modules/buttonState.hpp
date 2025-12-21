#pragma once

#include <string>
#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "ArduinoJson.h"

#include "magic_enum/magic_enum.hpp"
#include "debug.hpp"
#include "helpers.hpp"

#define DELAY_LONG_PRESS_MS 700
#define DELAY_DEBOUNCE_MS   50

#define FIRED               0
#define NO_CALLBACK         1
#define LOCKED              2
#define ACCES_DENIED        3
#define DUPLICATE           4
#define BOUNCE              5
#define FAILED              6
#define INVALID             7

class HMI;

enum class ButtonEventType {
    Pressed,
    LongPressed,
    Released,
    LongReleased
};

enum class ButtonName {
    MasterLock,
    Mode,
    Action,
    ValueChange,
    MotorTrigger,
    ERROR_INVALID
};

typedef struct ButtonEvent {
    ButtonEventType type;
    ButtonName name;
} buttonEvent_t;

class ButtonState {
    static void timerCallback(void* arg);

    public:
                    ButtonState(JsonVariantConst obj, const bool *lock);
                    ~ButtonState();
        uint8_t     trigger(bool);
        void        setCallBack(QueueHandle_t);
        bool        isValid() const;
        ButtonName  getName() const;

    private:
        void        onLongPress();
        void        fireLongPressTimer();
        bool        lock();
        bool        unlock();
        uint8_t     release();
        uint8_t     fireEvent(ButtonEventType) const;

        int                 gpio;
        ButtonName          name;
        bool                isPressed;
        unsigned long       wentDown;
        unsigned long       lastChange;
        SemaphoreHandle_t   mutex;
        esp_timer_handle_t  longPressTimer;
        QueueHandle_t       callback;
        const bool          *masterLock;
        bool                lockable;
        std::atomic<bool>   stopTasks;
};
