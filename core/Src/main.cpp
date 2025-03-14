#include "main.h"


#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#include "esp_log.h"

#define LOG_TAG "MAIN"

static Main my_main;// what does static mean here?

esp_err_t Main::setup(void)
{
    esp_err_t status {ESP_OK};
    ESP_LOGI(LOG_TAG, "In my main.setup(), trying to set up ultrasonic pins");

    ESP_LOGI(LOG_TAG,"Setting trigger pin output");

    status |= triggerPin.init();

    ESP_LOGI(LOG_TAG,"Setting echo pin input");
    status |= echoPin.init();
    // status |= ledGreen.init();
    // status |= ledBlue.init();

    //status |= wifi.init();

    // if (ESP_OK == status) {
    //     status |= wifi.begin();
    // }
    //status |= sntp.init();
 


    return status;
}

void Main::loop(void)
{
    ESP_LOGI(LOG_TAG, "Hello World!");

    ESP_LOGI(LOG_TAG, "Led on");
    //led.set(true);
    vTaskDelay(pdSECOND);
    ESP_LOGI(LOG_TAG, "Led off");
    //led.set(false);
    vTaskDelay(pdSECOND);
}

void Main::toggleRed() {
    // task function
  for(;;)
  {
    ESP_LOGI(LOG_TAG,"TOGGLING RED!!!");// not sure if this flushes anything
    ledRed.set(true);
    vTaskDelay(12000/portTICK_PERIOD_MS);// 12s just red
    ledRed.set(false);
    vTaskDelay(13000/portTICK_PERIOD_MS);// 13s no red
  }

}

void Main::toggleGreen() {
  for(;;)
  {
    ESP_LOGI(LOG_TAG,"TOGGLING Green!!!");
    vTaskDelay(12000/portTICK_PERIOD_MS);// nothing should happen for first 12s
	  ledGreen.set(true);
    vTaskDelay(10000/portTICK_PERIOD_MS);// 10s green on
    ledGreen.set(false);
    vTaskDelay(3000/portTICK_PERIOD_MS);// 3s green off
  }
}

void Main::toggleBlue() {
  for(;;)
  {
    ESP_LOGI(LOG_TAG,"TOGGLING Blue!!!");
    vTaskDelay(10000/portTICK_PERIOD_MS);// nothing should happen for 10s
    ledBlue.set(true);
    vTaskDelay(2000/portTICK_PERIOD_MS);// 2s led blue on
    ledBlue.set(false);
    vTaskDelay(10000/portTICK_PERIOD_MS);
    ledBlue.set(true);
    vTaskDelay(3000/portTICK_PERIOD_MS);
    // put it off here
    ledBlue.set(false);
  }
    
}

void Main::startUltraSonicSensor() {
  for (;;) {
    // Send a short HIGH pulse to the TRIG pin
    triggerPin.set(false);
    vTaskDelay(pdSECOND);// 1s delay
    triggerPin.set(true);
    esp_rom_delay_us(10);// this is to have 10 microseconds
    triggerPin.set(false);

    // now, we need to measure the pulse width from the ECHO pin
    int64_t start_time = esp_timer_get_time();// start a chrono

    while (echoPin.state() == false) {
      if ((esp_timer_get_time() - start_time) > 20000) { // timeout, it should have been high at a certain point
        ESP_LOGI(LOG_TAG,"Echo timeout. EchoPin cant receive the high pulse");
        break;
      }
    }

    // if we are here, it means that the sensor received the bouned off pulse

    start_time = esp_timer_get_time();
    while (echoPin.state() == true) {
      if ((esp_timer_get_time() - start_time) > 20000) {
        ESP_LOGI(LOG_TAG,"Echo timeout. EchoPin cant receive the low pulse");
        break;
      }
    }

    int64_t end_time = esp_timer_get_time();

    // lets calculate the time period, (time taken to do a round trip from the water and back)
    int64_t duration = end_time - start_time;// suposedly, the duration is in microsec

    // using this duration, lets calculate the distance

    float distance = (duration * SOUND_SPEED)/2;// in cm

    // Print the values to the console

    ESP_LOGI(LOG_TAG,"Time Period (us): %lld | Distance(cm):%.2f\n",duration,distance);

    vTaskDelay(pdSECOND);// wait 1 s before next reading




  }
  




}

void toggleRedWrapper(void* pvParameters) {
    ESP_LOGI(LOG_TAG,"I am in the red wrapper");
    my_main.toggleRed();

}

void toggleGreenWrapper(void* pvParameters) {
    my_main.toggleGreen();
}

void toggleBlueWrapper(void* pvParameters) {
    my_main.toggleBlue();
}

void ultrasonicTaskWrapper(void* pvParameters) {
  ESP_LOGI(LOG_TAG,"I am in the ultrasonicTaskWrapper");
  my_main.startUltraSonicSensor();
}


extern "C" void app_main(void)
{
    // we are starting from here 
    ESP_LOGI(LOG_TAG,"Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(LOG_TAG,"Initializing NVS");// not sure if this flushes anything

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(my_main.setup());
    // not doing UART now
    // Definitions for tasks
    // Task 1: Control Red light
    // if (xTaskCreate(toggleRedWrapper, "tRed", 2048, NULL, tskIDLE_PRIORITY, NULL) != pdPASS) {
    //     ESP_LOGI(LOG_TAG,"Problem Red");
    //     // ESP_LOGI(LOG_TAG,"Problem Red");

    // }
    // if (xTaskCreate(toggleGreenWrapper, "tGreen", 2048, NULL, tskIDLE_PRIORITY, NULL) != ESP_OK) {
    //     ESP_LOGI(LOG_TAG,"Problem Green");
    // }
    // if (    xTaskCreate(toggleBlueWrapper, "tBlue", 2048, NULL, tskIDLE_PRIORITY, NULL) != ESP_OK) {
    //     ESP_LOGI(LOG_TAG,"Problem Green");
    // }

    ESP_LOGI(LOG_TAG,"Creating task for ultrasonic sensor");

    if (xTaskCreate(ultrasonicTaskWrapper,"tUltraSonic",2048,NULL,tskIDLE_PRIORITY, NULL) != ESP_OK) {
      ESP_LOGI(LOG_TAG,"Problem happened while creating the ultrasonic sensor");
    }

    ESP_LOGI(LOG_TAG,"Evth fine");

    // while (true)
    // {
    //     my_main.loop();
    // }
}