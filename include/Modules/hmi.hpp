#pragma once

#include <unordered_map>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <ArduinoJson.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "sdkconfig.h"

#include "magic_enum/magic_enum.hpp"
#include "Modules/buttonState.hpp"
#include "Modules/outputManager.hpp"
#include "Modules/routine.hpp"
#include "Events/event.hpp"
#include "Events/eventFactory.hpp"
#include "debug.hpp"
#include "helpers.hpp"
#include "Outputs/output.hpp"

#define RESET                   0
#define WATCHDOG_TIMEOUT_MS (CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000 - 200)

class HmiEvent {
    public:
        int     pin;
        bool    isPressed;
};

enum ProgramMode {
    OneTime = 0, // Trigger new feeder event with current parameters
    ReplaceNext, // Next action call triggers next feeder event with current value
    NextAsIs = 3 // Next ation call triggers next feeder event in queue with its own settings
};

// TODO : auto lock after delay!

class HMI : public Observable<OutputEvent> {
    public:
                        HMI() = delete;
                        HMI(const JsonDocument&, Routine*);
        void            run();

        static void     hardwareListenerTask(void *);
        static void     buttonListenerTask(void *);
        static void     ledBlinkerTask(void *);
        static void     IRAM_ATTR gpioISRInterrupt(void *);

    private:
        void            toggleLock(ButtonEventType);
        void            valueChange(ButtonEventType);
        void            modeChange(ButtonEventType);
        void            action(ButtonEventType);
        void            motorFeedBack(ButtonEventType);
        void            setPgmValue(uint8_t);
        void            setLock(bool);
        void            setMode(ProgramMode);
        void            stateChanged() const;

        static QueueHandle_t                        hardwareEventQueue;

        std::unordered_map<int, ButtonState*>       buttons;
        QueueHandle_t                               buttonEventQueue;
        bool                                        lock;
        bool                                        running;
        uint8_t                                     pgmValue;
        ProgramMode                                 mode;
        TaskHandle_t                                hardwareTask;
        TaskHandle_t                                buttonTask;
        TaskHandle_t                                blinkTask;
        Routine                                     *routine;
        bool                                        blink;
        SemaphoreHandle_t                           blinkTex;
};
