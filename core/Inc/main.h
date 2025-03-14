#pragma once

// including ESP_IDF related headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define pdSECOND pdMS_TO_TICKS(1000)
#define SOUND_SPEED 0.0343 // speed of sound in cm/microsec
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_timer.h"


// including our custom headers
#include "Gpio.h"
#include "Wifi.h"
#include "SntpTime.h"

class Main final
{
public:
    // Main(void) :
    //     sntp {SNTP::Sntp::get_instance()} {}
    //Main(void) {};
    Main(void) {};
    esp_err_t setup(void);
    void loop(void);
    void toggleRed();
    void toggleGreen();
    void toggleBlue();

    void startUltraSonicSensor();


    //Gpio::GpioOutput led {GPIO_NUM_23,false};
    Gpio::GpioOutput ledRed {GPIO_NUM_23,false};
    Gpio::GpioOutput ledGreen {GPIO_NUM_18,false};
    Gpio::GpioOutput ledBlue {GPIO_NUM_17,false}; // meaning orange
    //WIFI::Wifi wifi;
    //SNTP::Sntp& sntp;

    Gpio::GpioOutput triggerPin {GPIO_NUM_13,false};// TRIGGER OUTPUT
    Gpio::GpioInput echoPin {GPIO_NUM_12,false};// ECHO INPUT
};