#include "hmi_test.hpp"

static const char *TAG = "hmi_test";

/* ERROR ORDERING
// acces denied (semaphore)
// lock
// dup
// bounce
// callback
// queue fail
*/

void template_test() {
    JsonDocument    json;
    std::string     str;
    bool            lock = false;
    QueueHandle_t   queue = xQueueCreate(20, sizeof(ButtonEvent));
    ButtonEvent     evt;

    str = "{\"type\":\"Action\",\"gpio\":8,\"lockable\":false}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));

    /*
    ** TEST HERE
    */

    vQueueDelete(queue);
}

void default_button_ctor() {
    // `is_constructible_v` returns false if this ctor is deleted
    TEST_ASSERT_FALSE(std::is_default_constructible_v<ButtonState>);
}

void no_double_trigger() {
    JsonDocument    json;
    std::string     str;
    bool            lock = false;
    QueueHandle_t   queue = xQueueCreate(100, sizeof(ButtonEvent));

    str = "{\"type\":\"Action\",\"gpio\":8}";
    deserializeJson(json, str);

    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 5));
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(true));

    vQueueDelete(queue);
}

void debounce() {
    JsonDocument    json;
    std::string     str;
    bool            lock = false;
    QueueHandle_t   queue = xQueueCreate(100, sizeof(ButtonEvent));

    str = "{\"type\":\"Action\",\"gpio\":8}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    TEST_ASSERT_EQUAL_UINT8(BOUNCE, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));

    vQueueDelete(queue);
}

void no_callback() {
    JsonDocument    json;
    std::string     str;
    bool            lock = false;

    str = "{\"type\":\"Action\",\"gpio\":8}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(NO_CALLBACK, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(NO_CALLBACK, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(NO_CALLBACK, btn.trigger(true));
}

void queue_full() {
    JsonDocument    json;
    std::string     str;
    bool            lock = false;
    QueueHandle_t   queue = xQueueCreate(2, sizeof(ButtonEvent));

    str = "{\"type\":\"Action\",\"gpio\":8}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FAILED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FAILED, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FAILED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FAILED, btn.trigger(false));

    vQueueDelete(queue);
}

void queue_full_and_emptied() {
    JsonDocument    json;
    std::string     str;
    bool            lock = false;
    QueueHandle_t   queue = xQueueCreate(2, sizeof(ButtonEvent));
    ButtonEvent     evt;

    str = "{\"type\":\"Action\",\"gpio\":8}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FAILED, btn.trigger(true));
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)));
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FAILED, btn.trigger(false));
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)));
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)));

    vQueueDelete(queue);
}

void button_event_types() {
    JsonDocument    json;
    std::string     str;
    bool            lock = false;
    QueueHandle_t   queue = xQueueCreate(20, sizeof(ButtonEvent));
    ButtonEvent     evt;

    str = "{\"type\":\"Action\",\"gpio\":8}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    ESP_LOGE(TAG, "checking button event type : PRESS");
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));                       //PRESS
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)));
    TEST_ASSERT_EQUAL(btn.getName(), evt.name);
    TEST_ASSERT_EQUAL(ButtonEventType::Pressed, evt.type);

    ESP_LOGE(TAG, "checking button event type :  LONG PRESS + RELEASE");
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(DELAY_LONG_PRESS_MS * 2))); // LONG PRESS + RELEASE
    TEST_ASSERT_EQUAL(btn.getName(), evt.name);
    TEST_ASSERT_EQUAL(ButtonEventType::LongPressed, evt.type);
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)));
    TEST_ASSERT_EQUAL(btn.getName(), evt.name);
    TEST_ASSERT_EQUAL(ButtonEventType::LongReleased, evt.type);

    ESP_LOGE(TAG, "checking button event type :  PRESS + RELEASE");
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));                       // PRESS + RELEASE
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)));
    TEST_ASSERT_EQUAL(btn.getName(), evt.name);
    TEST_ASSERT_EQUAL(ButtonEventType::Pressed, evt.type);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));
    TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)));
    TEST_ASSERT_EQUAL(btn.getName(), evt.name);
    TEST_ASSERT_EQUAL(ButtonEventType::Released, evt.type);

    ESP_LOGE(TAG, "checking button event type : DONE");
    vQueueDelete(queue);
}

void button_types() {
    JsonDocument    json;
    std::string     str;
    bool            lock = false;
    QueueHandle_t   queue = xQueueCreate(20, sizeof(ButtonEvent));
    ButtonEvent     evt;

    for (std::string_view name : magic_enum::enum_names<ButtonName>()) {
        if (name.contains("ERROR")) {
            continue;
        }
        str = "{\"type\":\"" + std::string(name) + "\",\"gpio\":8}";
        deserializeJson(json, str);
        ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
        btn.setCallBack(queue);
        vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
        TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
        TEST_ASSERT_EQUAL(pdTRUE, xQueueReceive(queue, &evt, pdMS_TO_TICKS(100)));
        TEST_ASSERT_EQUAL(btn.getName(), evt.name);
    }

    str = "{\"type\":\"WRONG_TYPE\",\"gpio\":8}";
    deserializeJson(json, str);

    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    TEST_ASSERT_EQUAL(false, btn.isValid());
    TEST_ASSERT_EQUAL(INVALID, btn.trigger(true));

    vQueueDelete(queue);
}

void lockable() {
    JsonDocument    json;
    std::string     str;
    bool            lock = true;
    QueueHandle_t   queue = xQueueCreate(100, sizeof(ButtonEvent));

    str = "{\"type\":\"Action\",\"gpio\":8,\"lockable\":true}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));

    TEST_ASSERT_EQUAL_UINT8(LOCKED, btn.trigger(true));
    lock = false;
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    lock = true;
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(LOCKED, btn.trigger(false));
    lock = false;
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));
    lock = true;
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(LOCKED, btn.trigger(true));

    vQueueDelete(queue);
}

void combined() {
    JsonDocument    json;
    std::string     str;
    bool            lock = false;
    QueueHandle_t   queue = xQueueCreate(100, sizeof(ButtonEvent));

    str = "{\"type\":\"Action\",\"gpio\":8}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true)); //first press => btn is down
    for (int i = 0; i < 60; i++) {
        TEST_ASSERT_EQUAL_UINT8(BOUNCE, btn.trigger(false)); //bounce -> no change
        TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(true)); //dup is checked before bounce
    }
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false)); //debounced -> btn is up
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(false)); //duplicate -> no change
    TEST_ASSERT_EQUAL_UINT8(BOUNCE, btn.trigger(true)); //bounce -> no change
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true)); //press => btn is down
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false)); //debounced -> btn is up
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(false)); //duplicate
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(false)); //duplicate
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    TEST_ASSERT_EQUAL_UINT8(BOUNCE, btn.trigger(false)); //bounce -> no change
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(true));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));

    vQueueDelete(queue);
}

void combined_not_lockable() {
    JsonDocument    json;
    std::string     str;
    bool            lock = true;
    QueueHandle_t   queue = xQueueCreate(100, sizeof(ButtonEvent));

    str = "{\"type\":\"Action\",\"gpio\":8,\"lockable\":false}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true)); //first press => btn is down
    for (int i = 0; i < 60; i++) {
        TEST_ASSERT_EQUAL_UINT8(BOUNCE, btn.trigger(false)); //bounce -> no change
        TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(true)); //dup is checked before bounce
    }
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false)); //debounced -> btn is up
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(false)); //duplicate -> no change
    TEST_ASSERT_EQUAL_UINT8(BOUNCE, btn.trigger(true)); //bounce -> no change
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true)); //press => btn is down
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false)); //debounced -> btn is up
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(false)); //duplicate
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(false)); //duplicate
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));

    vQueueDelete(queue);
}

void combined_lockable() {
    JsonDocument    json;
    std::string     str;
    bool            lock = true;
    QueueHandle_t   queue = xQueueCreate(100, sizeof(ButtonEvent));

    str = "{\"type\":\"Action\",\"gpio\":8,\"lockable\":true}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    btn.setCallBack(queue);
    TEST_ASSERT_EQUAL_UINT8(LOCKED, btn.trigger(true));
    lock = false;
    for (int i = 0; i < 60; i++) {
        TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(false));
        TEST_ASSERT_EQUAL_UINT8(BOUNCE, btn.trigger(true));
    }
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(false));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    lock = true;
    TEST_ASSERT_EQUAL_UINT8(LOCKED, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(LOCKED, btn.trigger(true));
    TEST_ASSERT_EQUAL_UINT8(LOCKED, btn.trigger(true));

    vQueueDelete(queue);
}

void error_order_check() {
    JsonDocument    json;
    std::string     str;
    bool            lock = true;
    QueueHandle_t   queue = xQueueCreate(1, sizeof(ButtonEvent));

    str = "{\"type\":\"Action\",\"gpio\":8,\"lockable\":true}";
    deserializeJson(json, str);
    ButtonState btn = ButtonState(json.as<JsonVariantConst>(), &lock);
    TEST_ASSERT_EQUAL_UINT8(LOCKED, btn.trigger(false));
    TEST_ASSERT_EQUAL_UINT8(LOCKED, btn.trigger(true));
    lock = false;
    TEST_ASSERT_EQUAL_UINT8(DUPLICATE, btn.trigger(false));
    TEST_ASSERT_EQUAL_UINT8(BOUNCE, btn.trigger(true));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(NO_CALLBACK, btn.trigger(true));
    btn.setCallBack(queue);
    TEST_ASSERT_EQUAL_UINT8(BOUNCE, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FIRED, btn.trigger(false));
    vTaskDelay(pdMS_TO_TICKS(DELAY_DEBOUNCE_MS * 2));
    TEST_ASSERT_EQUAL_UINT8(FAILED, btn.trigger(true));

    vQueueDelete(queue);
}

void runUnityTests(void *arg)
{
    UNITY_BEGIN();

    RUN_TEST(default_button_ctor);
    RUN_TEST(no_double_trigger);
    RUN_TEST(debounce);
    RUN_TEST(no_callback);
    RUN_TEST(queue_full);
    RUN_TEST(queue_full_and_emptied);
    RUN_TEST(button_event_types);
    RUN_TEST(button_types);
    RUN_TEST(lockable);
    RUN_TEST(combined);
    RUN_TEST(combined_not_lockable);
    RUN_TEST(combined_lockable);
    RUN_TEST(error_order_check);

    UNITY_END();
}

#ifdef UNIT_TEST

extern "C" int app_main(void)
{
    if (!xTaskGetSchedulerState()) {
        vTaskStartScheduler();
    }
    xTaskCreate(runUnityTests, "UnityTests", 8192, NULL, 5, NULL);

    return 0;
}

#endif
