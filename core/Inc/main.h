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






// including our custom headers
#include "Gpio.h"
#include "Wifi.h"
#include "SntpTime.h"

#define pdSECOND pdMS_TO_TICKS(1000)
#define SOUND_SPEED 0.0343 // speed of sound in cm/microsec

#define RADIUS_BOTTLE_BASE 4 // cm
#define QUEUE_SIZE 5

#define MEN_INTAKE 3700 //ml
#define WOMEN_INTAKE 2700 //ml

QueueHandle_t dataQueueDistance;
QueueHandle_t dataQueueVolume;
constexpr double PI {3.14159265};

constexpr double surface_base {RADIUS_BOTTLE_BASE * RADIUS_BOTTLE_BASE * PI};// cm2

constexpr int max_bottle_distance {20};//cm   

constexpr double error_margin {0.6};//cm

class Main final
{
public:
    // Main(void) :
    //     sntp {SNTP::Sntp::get_instance()} {}
    Main(void) {};
    esp_err_t setup(void);

    void startUltraSonicSensor();

    void calculateVolume();

    void fireLeds();

    void sendVolume();

    //esp_http_client_config_t config;

    Gpio::GpioOutput ledRed {GPIO_NUM_32,false};
    Gpio::GpioOutput ledGreen {GPIO_NUM_33,false};
    Gpio::GpioOutput ledOrange {GPIO_NUM_25,false}; // meaning orange
    WIFI::Wifi wifi {};
    //SNTP::Sntp& sntp;// TODO, later on, uncomment

    Gpio::GpioOutput triggerPin {GPIO_NUM_13,false};// TRIGGER OUTPUT
    Gpio::GpioInput echoPin {GPIO_NUM_12,false};// ECHO INPUT

    esp_http_client_config_t config {
        .url = FIREBASE_URL,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client {};
};