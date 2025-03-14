#pragma once

#include "esp_wifi.h"

#include "freertos/semphr.h"

#include <mutex>

#include <cstring>
#include <algorithm>

// setting up a wifi station, from documentation
#include "esp_event.h"
#include "esp_log.h"

#define pdSECOND pdMS_TO_TICKS(1000)

namespace WIFI
{

class Wifi
{
    constexpr static const char* _log_tag {"WiFi"};
    constexpr static const char* ssid {"HUAWEI Y9s"};
    constexpr static const char* password{"22ac5f6da38e"};

public:

    enum class state_e
    {//strongly typed enum, number is meaningless, word is important

        NOT_INITIALISED,
        INITIALISED,
        READY_TO_CONNECT,//doesn't say if network exists or if credentials are correct. Attempt to connect to the network
        CONNECTING,
        WAITING_FOR_IP,//in between states, connected to the constructor but not been assigned an ip address
        CONNECTED,
        DISCONNECTED,
        ERROR
    };

    // constructors. Rule of 5.
    Wifi(void);
    ~Wifi(void)                     = default;
    Wifi(const Wifi&)               = default;
    Wifi(Wifi&&)                    = default;
    Wifi& operator=(const Wifi&)    = default;
    Wifi& operator=(Wifi&&)         = default;

    esp_err_t init(void); // set everything up
    esp_err_t begin(void); // start wifi, connect,etc

    // _state is static so get_state should be static too
    constexpr static const state_e& get_state(void) { return _state; }; //initialized, not initialized, connected, not, got ip address, not an ip address. return by reference so that we won't change the state.
    constexpr static const char* get_mac() 
        {return mac_addr_cstr;} // static because this function will always return the same thing, regardless of the instance I am in.

private:
    static esp_err_t _init(void);
    void state_machine(void);// internal machine
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    static wifi_init_config_t wifi_init_config;
    static wifi_config_t wifi_config;
    static state_e _state;
    static std::mutex init_mutex; // making sure only one thread accesses the constructor at a time
    static std::mutex connect_mutex; // only one thread should be connecting at a time, this mutex is making sure it's the case
    static std::mutex state_mutex; // only one thread should change the value of state
    static char mac_addr_cstr[13];// all instances of this class share the same mac address.
    static esp_err_t _get_mac(void); // the user should NOT call this function directly, and it is the same for all instances
};

}// namespace WIFI