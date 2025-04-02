#include "main.h"


#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
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
    ESP_LOGI(LOG_TAG,"Setting led pins");
    status |= ledRed.init();
    status |= ledOrange.init();
    status |= ledGreen.init();


    return status;
}

void Main::startUltraSonicSensor() {
  double distance {};
  bool received_pulse_echo_high;
  bool received_pulse_echo_low;
  for (;;) {
    vTaskDelay(40 * pdSECOND);
    received_pulse_echo_high = true;
    received_pulse_echo_low = true;
    ESP_LOGI(LOG_TAG,"start t2");
    // Send a short HIGH pulse to the TRIG pin
    triggerPin.set(false);
    vTaskDelay(pdSECOND);// 1s delay
    triggerPin.set(true);
    esp_rom_delay_us(10);// this is to have 10 microseconds
    triggerPin.set(false);

    // now, we need to measure the pulse width from the ECHO pin
    int64_t start_time = esp_timer_get_time();// start a chrono, microseconds

    while (echoPin.state() == false) {
      if ((esp_timer_get_time() - start_time) > 20000) { // timeout, it should have been high at a certain point
        ESP_LOGI(LOG_TAG,"Echo timeout. EchoPin cant receive the high pulse");
        received_pulse_echo_high = false;
        received_pulse_echo_low = false;
        break;
      }
    }

    // if we are here, it means that the sensor received the bounced off pulse, (not really)

    if (received_pulse_echo_high) {
      // process only if you received the high pulse in the estimated time frame
      start_time = esp_timer_get_time();
      while (echoPin.state() == true) {
        if ((esp_timer_get_time() - start_time) > 20000) {
          ESP_LOGI(LOG_TAG,"Echo timeout. EchoPin cant receive the low pulse");
          received_pulse_echo_low = false;
          break;
        }
      }
    }

    if (received_pulse_echo_low) {
      // only process if you received sth in the specified time frame
      int64_t end_time = esp_timer_get_time();

      // lets calculate the time period, (time taken to do a round trip from the water and back)
      int64_t duration = end_time - start_time;// supposedly, the duration is in microsec

      // using this duration, lets calculate the distance

      distance = (duration * SOUND_SPEED)/2;// in cm

      // Print the values to the console

      ESP_LOGI(LOG_TAG,"Time Period (us): %lld | Distance(cm):%.2f\n",duration,distance);

      // check if queue is full
      if (uxQueueSpacesAvailable(dataQueueDistance) == 0) {
        xQueueReset(dataQueueDistance);// clear the queue
      }

      // send the data
      if (xQueueSend(dataQueueDistance, &distance, pdMS_TO_TICKS(100)) == pdTRUE) {
        ESP_LOGI("TASK 2", "Sent: Value=%.2f", distance);
      } else {
        ESP_LOGE("TASK 2", "Error happened while sending distance!");
      }
    }
    ESP_LOGI(LOG_TAG,"end t2");
    // vTaskDelay(2*pdSECOND);// wait 2s before next reading, this will change depending on the whole cycle TODO
  }
}

void Main::calculateVolume() {
  // vTaskDelay(pdSECOND);// 1s delay
  ESP_LOGI(LOG_TAG,"I am in the calculate volume function");
  double distanceReceivedNew {};// time t
  double distanceReceivedOld {};// time t-1
  double drankVolumeNew {};// total drank volume at an instant t
  double drankVolumeOld {};// toal drank volume at an instant t-1


  for (;;) {
    vTaskDelay(42 * pdSECOND);// 1s delay
    ESP_LOGI(LOG_TAG,"start t3");
    if (xQueueReceive(dataQueueDistance, &distanceReceivedNew, portMAX_DELAY) == pdTRUE) {
      ESP_LOGI("calculateVolume", "Received: Distance=%.2f", distanceReceivedNew);
      if ((distanceReceivedNew > max_bottle_distance) || (distanceReceivedNew < 0)) {
        ESP_LOGE("TASK 3", "Invalid distance!");
      } else if ((distanceReceivedOld + error_margin ) <= distanceReceivedNew) {
        drankVolumeNew = drankVolumeOld + ((distanceReceivedNew - distanceReceivedOld)*surface_base); //cm3, TODO, might need to remove the height of the lid itself
        drankVolumeOld = drankVolumeNew;
        distanceReceivedOld = distanceReceivedNew;
        ESP_LOGI(LOG_TAG,"the volume you drank in ml so far is: %.2f",drankVolumeNew);

        // check if queue is full
        if (uxQueueSpacesAvailable(dataQueueVolume) == 0) {
          xQueueReset(dataQueueVolume);// clear the queue
        }

        // check if queue is full
        if (uxQueueSpacesAvailable(dataQueueVolumeDb) == 0) {
          xQueueReset(dataQueueVolumeDb);// clear the queue
        }

        if (xQueueSend(dataQueueVolume, &drankVolumeNew, pdMS_TO_TICKS(100)) == pdTRUE) {
          ESP_LOGI("TASK 3", "Sent to led task: Volume=%.2f", drankVolumeNew);
        } else {
          ESP_LOGE("TASK 3", "Error happened while sending volume to led task!");
        }

        if (xQueueSend(dataQueueVolumeDb,&drankVolumeNew,pdMS_TO_TICKS(100)) == pdTRUE) {
          ESP_LOGI("TASK 3", "Sent to db task: Volume=%.2f", drankVolumeNew);
        } else {
          ESP_LOGE("TASK 3", "Error happened while sending volume to db task!");
        }

      } else if ((distanceReceivedOld - error_margin)>=distanceReceivedNew) {
        ESP_LOGW(LOG_TAG,"Refill detected!");
        distanceReceivedOld = distanceReceivedNew;
      } else { 
        ESP_LOGW(LOG_TAG,"Normal fluctuancies!");
      }
      // send to the leds task
      
    } else { 
      ESP_LOGE("calculateVolume", "Error receiving data");
    }
    ESP_LOGI(LOG_TAG,"end t3");


    // vTaskDelay(2*pdSECOND);//2s delays

  }
}

void Main::fireLeds() {
  // vTaskDelay(2*pdSECOND);
  double drankVolumeReceived {};


  for(;;) {
    vTaskDelay(44*pdSECOND);
    ESP_LOGI(LOG_TAG,"start t4");
    if (xQueueReceive(dataQueueVolume, &drankVolumeReceived, portMAX_DELAY) == pdTRUE) {
      ESP_LOGI("TASK 4", "Received: Volume Received=%.2f", drankVolumeReceived);

      if ((drankVolumeReceived < (WOMEN_INTAKE * 0.5))) {
        ESP_LOGI("TASK 4", "Setting red=%.2f", drankVolumeReceived);
        // we drank less than the half
        ledRed.set(true);// DANGER
        ledOrange.set(false);
        ledGreen.set(false);
        startBuzzer = true;
      } else if ((drankVolumeReceived >= (WOMEN_INTAKE * 0.5)) && (drankVolumeReceived < (WOMEN_INTAKE))) {
        // we drank at least the half but it's less than the 3/4
        ledOrange.set(true);
        ledRed.set(false);
        ledGreen.set(false);
        startBuzzer = true;
      } else {
        // we drank well
        ledOrange.set(false);
        ledRed.set(false);
        ledGreen.set(true);
        startBuzzer = false;
      }// TODO, might add another else, if we drank double what we needed, might be a value set by the user
    } else {
      ESP_LOGE("TASK 4", "Error receiving data");
    }
    ESP_LOGI(LOG_TAG,"end t4");
    // vTaskDelay(2*pdSECOND);
  }
}

void Main::sendVolume() {
  // vTaskDelay(6*pdSECOND);
  double drankVolumeReceived {};
  char volume[200];



  for (;;) {
    vTaskDelay(46*pdSECOND);

    char time [100];
  
    ESP_LOGI(LOG_TAG,"start t5");
    if (xQueueReceive(dataQueueVolumeDb,&drankVolumeReceived,portMAX_DELAY) == pdTRUE) {
      ESP_LOGI("TASK 5", "Received: Volume Received=%.2f", drankVolumeReceived);

      const char* asciiTime {sntp.ascii_time_now()};

      for (int i = 0; i < strlen(asciiTime);i++) {
        //ESP_LOGI("TASK 4", "Before sending the time is: %c", asciiTime[i]);
        time[i] = asciiTime[i];
        if (asciiTime[i] == '\n') {
          time[i] = '\0';
        }
      }
      // now we received the volume, send it to db
      snprintf(volume,sizeof(volume),"{\"volume_drank\": %.2f ,\"timestamp\": \"%s\"}", drankVolumeReceived,time);

      ESP_LOGI("TASK 5", "Before sending the volume  is: %s", volume);

      if (wifi.get_state() == WIFI::Wifi::state_e::CONNECTED) {
        esp_err_t err {ESP_OK};
        client  = {esp_http_client_init(&config)};
        err |= esp_http_client_set_header(client, "Content-Type", "application/json");

        if (err == ESP_OK) {
          esp_http_client_set_post_field(client, volume, strlen(volume));
          err |= esp_http_client_perform(client);
          if (err == ESP_OK) {
            ESP_LOGI(LOG_TAG, "Data sent to Firebase successfully");
          } else {
            ESP_LOGE(LOG_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
          }
        } else {
          ESP_LOGE("TASK 5","probelm in esp_http_client_set_header");
        }
        esp_http_client_cleanup(client);// TODO, destructor
      } else {
        ESP_LOGE("TASK 5", "Not connected to wifi :)");
      }
    } else {
      ESP_LOGE("TASK 5", "No information received!");
    }


    // vTaskDelay(6*pdSECOND);
  }
}

void Main::setupWifi() {

  esp_err_t status {ESP_OK};

  status |= wifi.init();

  if (ESP_OK == status) {
    status |= wifi.begin();
  }
    
  status |= sntp.init();

  while (!sntp.isSynched()) {
    ESP_LOGI(LOG_TAG,"Waiting one second to synchronize with ntp server");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  ESP_LOGI(LOG_TAG,"Before client");

 
  ESP_LOGI(LOG_TAG,"After client");

  for (;;) {
    // if (status  == ESP_OK) {
    //   ESP_LOGI(LOG_TAG,"Everything ran smoothly (sntp + wifi)");
    // }

  }
}

// void Main::triggerBuzzer() {
//   for (;;) {
//     vTaskDelay(51*pdSECOND);
//     ESP_LOGI(LOG_TAG,"trying to trigger buzzer");
//     if (startBuzzer) {
//       ESP_LOGI(LOG_TAG,"trying to set to true");
//       buzzer.set(true);
//       vTaskDelay(2*pdSECOND);
//       buzzer.set(false);
//     }
//   }

// }

void wifiSetupWrapper (void* pvParameters) {
  ESP_LOGI(LOG_TAG,"I am in the wifi set up wrapper");
  my_main.setupWifi();
}

void fireLedsWrapper(void* pvParameters) {
  ESP_LOGI(LOG_TAG,"I am in the fireLedsWrapper");
  my_main.fireLeds();
}

void calculateVolumeWrapper(void* pvParameters) {
  ESP_LOGI(LOG_TAG,"I am in the calculateVolumeWrapper");
  my_main.calculateVolume();
}

void ultrasonicTaskWrapper(void* pvParameters) {
  ESP_LOGI(LOG_TAG,"I am in the ultrasonicTaskWrapper");
  my_main.startUltraSonicSensor();
}

void sendDataWrapper (void* pvParameters) {
  ESP_LOGI(LOG_TAG, "I am in the send data wrapper task");
  my_main.sendVolume();
}

// void triggerBuzzerWrapper(void* pvParameters) {
//   ESP_LOGI(LOG_TAG,"I am in the trigger buzzer task");
//   my_main.triggerBuzzer();
// }


extern "C" void app_main(void)
{
    double distance {};
    double drankVolume {};
    // we are starting from here 
    ESP_LOGI(LOG_TAG,"Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(LOG_TAG,"Initializing NVS");

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(my_main.setup());
    //vTaskDelay(10000*pdSECOND);

    ESP_LOGI(LOG_TAG,"Do I even get here???");

    ESP_LOGI(LOG_TAG,"Creating a task for wifi set up");
    if (xTaskCreate(wifiSetupWrapper,"tWifi",4096,NULL,tskIDLE_PRIORITY, NULL) != pdPASS) {
      ESP_LOGI(LOG_TAG,"Problem happened while creating the wifi task");
    }

    ESP_LOGI(LOG_TAG,"Creating task for ultrasonic sensor");

    if (xTaskCreate(ultrasonicTaskWrapper,"tUltraSonic",2048,NULL,tskIDLE_PRIORITY, NULL) != pdPASS) {
      ESP_LOGI(LOG_TAG,"Problem happened while creating the ultrasonic sensor task");
    }

    ESP_LOGI(LOG_TAG,"Creating task for volume calculations");

    if (xTaskCreate(calculateVolumeWrapper,"tCalculateVolume",2048,NULL,tskIDLE_PRIORITY,NULL) != pdPASS) {
      ESP_LOGI(LOG_TAG,"Problem happened while creating the calculate volume task");
    }

    ESP_LOGI(LOG_TAG,"Creating a task to fire leds");
    if (xTaskCreate(fireLedsWrapper,"tFireLeds",2048,NULL,tskIDLE_PRIORITY,NULL) != pdPASS) {
      ESP_LOGI(LOG_TAG,"Problem happened while creating the fire leds task");
    }

    ESP_LOGI(LOG_TAG,"Creating a task to send data to the database");
    if (xTaskCreate(sendDataWrapper,"tSendData",4096,NULL,tskIDLE_PRIORITY,NULL) != pdPASS) {
      ESP_LOGI(LOG_TAG,"Problem happened while creating the send data task");
    }

    // ESP_LOGI(LOG_TAG,"Creating a buzzer task");
    // if (xTaskCreate(triggerBuzzerWrapper,"tTriggerBuzzer",2048,NULL,tskIDLE_PRIORITY,NULL) != pdPASS) {
    //   ESP_LOGI(LOG_TAG,"Problem happened while creating the trigger buzzer task");
    // }


    // distance queue
    dataQueueDistance = xQueueCreate(QUEUE_SIZE, sizeof(distance));

    // volume queue
    dataQueueVolume = xQueueCreate(QUEUE_SIZE, sizeof(drankVolume));

    // volume queue as well, but for the database send task

    dataQueueVolumeDb = xQueueCreate(QUEUE_SIZE,sizeof(drankVolume));

    if (dataQueueDistance == NULL) {
      ESP_LOGE(LOG_TAG, "Failed to create distance queue!");
      return;
    }

    if (dataQueueVolume == NULL) {
      ESP_LOGE(LOG_TAG, "Failed to create volume queue!");
      return;
    }

    if (dataQueueVolumeDb == NULL) {
      ESP_LOGE(LOG_TAG, "Failed to create volume queue for db task!");
      return;
    }


    ESP_LOGI(LOG_TAG,"Evth fine");
}