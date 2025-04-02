#pragma once

// including ESP_IDF related headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <esp_http_client.h>
#include "driver/ledc.h"
// #include "esp_mac.h"








// including our custom headers
#include "Gpio.h"
#include "Wifi.h"
#include "SntpTime.h"

#define pdSECOND pdMS_TO_TICKS(1000)
#define SOUND_SPEED 0.0343 // speed of sound in cm/microsec

#define RADIUS_BOTTLE_BASE 4.5 // cm
#define QUEUE_SIZE 5

#define MEN_INTAKE 3700 //ml
#define WOMEN_INTAKE 500 //ml

#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER   LEDC_TIMER_0

QueueHandle_t dataQueueDistance;
QueueHandle_t dataQueueVolume;
QueueHandle_t dataQueueVolumeDb;
constexpr double PI {3.14159265};

constexpr double surface_base {RADIUS_BOTTLE_BASE * RADIUS_BOTTLE_BASE * PI};// cm2

constexpr int max_bottle_distance {10};//cm   

constexpr double error_margin {0.1};// was 0.6 cm


class Main final
{
public:
    Main(void) :
        sntp {SNTP::Sntp::get_instance()} {}
    // Main(void) {};
    esp_err_t setup(void);
    // ~Main() {
    //     esp_http_client_cleanup(client);
    // }

    void startUltraSonicSensor();

    void calculateVolume();

    void fireLeds();

    void sendVolume();

    void setupWifi();

    void triggerBuzzer();

    //esp_http_client_config_t config;

    Gpio::GpioOutput ledRed {GPIO_NUM_32,false};
    Gpio::GpioOutput ledGreen {GPIO_NUM_33,false};
    Gpio::GpioOutput ledOrange {GPIO_NUM_25,false}; // meaning orange
    WIFI::Wifi wifi {};
    SNTP::Sntp& sntp;// TODO, later on, uncomment

    Gpio::GpioOutput triggerPin {GPIO_NUM_13,false};// TRIGGER OUTPUT
    Gpio::GpioInput echoPin {GPIO_NUM_12,false};// ECHO INPUT

    Gpio::GpioOutput buzzer {GPIO_NUM_26,false};

    bool startBuzzer {false};

    esp_http_client_config_t config {
        .url = FIREBASE_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000
    };

    esp_http_client_handle_t client {};

    // // Configure PWM for buzzer
    // ledc_timer_config_t ledc_timer = {
    //     .speed_mode       = LEDC_HIGH_SPEED_MODE,
    //     .duty_resolution  = LEDC_TIMER_10_BIT,
    //     .timer_num        = LEDC_TIMER,
    //     .freq_hz          = 4000,  // Default frequency
    //     .clk_cfg          = LEDC_AUTO_CLK
    // };
    // ledc_timer_config(&ledc_timer);
    // // ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));


    // ledc_channel_config_t ledc_channel = {
    //     .gpio_num   = GPIO_NUM_26,
    //     .speed_mode = LEDC_HIGH_SPEED_MODE,
    //     .channel    = LEDC_CHANNEL,
    //     .intr_type  = LEDC_INTR_DISABLE,
    //     .timer_sel  = LEDC_TIMER,
    //     .duty       = 0,  // Start with 0 duty
    //     .hpoint     = 0
    // };
    // ledc_channel_config(&ledc_channel);
};