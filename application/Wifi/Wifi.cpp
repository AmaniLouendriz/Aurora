#include "Wifi.h"
#include <esp_mac.h>
#include <esp_log.h>
#include <mutex>

namespace WIFI
{

char                Wifi::mac_addr_cstr[] {};
std::mutex          Wifi::init_mutex {};
std::mutex          Wifi::connect_mutex {};
std::mutex          Wifi::state_mutex {};
Wifi::state_e       Wifi::_state {state_e::NOT_INITIALISED};
wifi_init_config_t  Wifi::wifi_init_config = WIFI_INIT_CONFIG_DEFAULT(); // TODO whats wrong here
wifi_config_t       Wifi::wifi_config {};

Wifi::Wifi()
{
    ESP_LOGI(_log_tag, "%s: Waiting for init_mutex", __func__);// __func__ gives the name of the function I am in
    // only one thread should get here at a time
    std::lock_guard<std::mutex> guard(init_mutex);

    if (!get_mac()[0]) { // mac address is still the way we initialized it
        if (ESP_OK != _get_mac()) {
            esp_restart();
        }
    }// if not, then we already know mac address, no need to call again.
}

// esp_wifi_start calls event_handler, and it is sending it a WIFI_EVENT
void Wifi::event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (WIFI_EVENT == event_base) {
        ESP_LOGI(_log_tag, "%s:%d Got a WIFI_EVENT", __func__,__LINE__);
        return wifi_event_handler(arg,event_base,event_id,event_data);
    }

    else if (IP_EVENT == event_base) {
        ESP_LOGI(_log_tag, "%s:%d Got an IP_EVENT", __func__,__LINE__);
        return ip_event_handler(arg,event_base,event_id,event_data);
    }

    else {
        ESP_LOGE(_log_tag, "%s:%d Unexpected event: %s", __func__,__LINE__, event_base);
    }
}

void Wifi::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (WIFI_EVENT == event_base) {
        const  wifi_event_t event_type {static_cast<wifi_event_t>(event_id)};
        ESP_LOGI(_log_tag, "%s:%d : Event ID %ld", __func__,__LINE__,event_id);

        // in this switch, we would end up in state41 if the credentials of the wifi aren't valid.UPDATE: even if credentials are valid, first wifi event is always state 41.
        switch(event_type)
        {
            case WIFI_EVENT_STA_START:
            {
                ESP_LOGI(_log_tag, "%s:%d : STA_START, waiting for state_mutex", __func__,__LINE__);// critical section because we are changing the state, there is a thread that makes a wifi event, if no guards, then we would end up with two threads modifying the same state variable, which is not what we want.
                std::lock_guard<std::mutex> state_guard(state_mutex);// recursive mutex??
                _state = state_e::READY_TO_CONNECT;
                ESP_LOGI(_log_tag, "%s:%d : READY_TO_CONNECT", __func__,__LINE__);
                break;
            }
            case WIFI_EVENT_STA_CONNECTED: // we are connected to the router, that doesn't mean is that the wifi is ready to use. because we don't have an ip yet. That doens't mean I have 
            // internet or network access
            {  
                ESP_LOGI(_log_tag, "%s:%d : STA_CONNECTED, waiting for state_mutex", __func__,__LINE__);  
                std::lock_guard<std::mutex> state_guard(state_mutex);// recursive mutex??
                ESP_LOGI(_log_tag, "%s:%d : WAITING_FOR_IP", __func__,__LINE__);
                _state = state_e::WAITING_FOR_IP;
                break;
            }
            default:
                ESP_LOGW(_log_tag, "%s:%d : Default switch case (%ld)", __func__,__LINE__, event_id);// want here
                break;
        }
    }
}

void Wifi::ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    const  ip_event_t event_type {static_cast<ip_event_t>(event_id)};
    ESP_LOGI(_log_tag, "%s:%d : Event ID %ld", __func__,__LINE__,event_id);

    switch(event_type)
    {
        case IP_EVENT_STA_GOT_IP:
        {
            ESP_LOGI(_log_tag, "%s:%d : Got IP, waiting for state_mutex", __func__,__LINE__);
            std::lock_guard<std::mutex> state_guard(state_mutex);// recursive mutex??
            _state = state_e::CONNECTED;
            ESP_LOGI(_log_tag, "%s:%d : Connected!", __func__,__LINE__);
            break;
        }
        case IP_EVENT_STA_LOST_IP:
        {
            ESP_LOGI(_log_tag, "%s:%d : Lost IP, waiting for state_mutex", __func__,__LINE__);
            std::lock_guard<std::mutex> state_guard(state_mutex);// recursive mutex??
            _state = state_e::WAITING_FOR_IP;
            ESP_LOGI(_log_tag, "%s:%d : Waiting for IP", __func__,__LINE__);
            break;
        }
        default:
            ESP_LOGW(_log_tag, "%s:%d : Default switch case (%ld)",__func__,__LINE__,event_id);
            break;
    }
}



esp_err_t Wifi::init(void)
{
    return _init();
}

esp_err_t Wifi::_init(void)
{
    ESP_LOGI(_log_tag, "%s:%d : Waiting for init_mutex:", __func__,__LINE__);
    // this is being called once and once per thread
    std::lock_guard<std::mutex> init_guard(init_mutex);

    esp_err_t status {ESP_OK};

    ESP_LOGI(_log_tag, "%s:%d : Waiting for state_mutex:", __func__,__LINE__);
    std::lock_guard<std::mutex> state_guard(state_mutex);

    if (state_e::NOT_INITIALISED == _state) {
        // one time initialization
        ESP_LOGI(_log_tag, "%s:%d : Calling esp_netif_init", __func__,__LINE__);
        status = esp_netif_init();
        ESP_LOGI(_log_tag, "%s:%d : esp_netif_init:%s", __func__,__LINE__, esp_err_to_name(status));
        if (ESP_OK == status) {
            ESP_LOGI(_log_tag, "%s:%d : Calling esp_netif_create_default_wifi_sta", __func__,__LINE__);
            const esp_netif_t* const p_netif =  esp_netif_create_default_wifi_sta();
            ESP_LOGI(_log_tag, "%s:%d : esp_netif_create_default_wifi_sta:%p", __func__,__LINE__, p_netif);// if a nullptr is returned in p_netif, then it fails

            if (!p_netif) {
                status = ESP_FAIL;
            }
        }
        // default configuration for Wifi instance

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d : Calling esp_wifi_init", __func__,__LINE__);
            status = esp_wifi_init(&wifi_init_config); // pass by address
            ESP_LOGI(_log_tag, "%s:%d : esp_wifi_init:%s", __func__,__LINE__, esp_err_to_name(status));
        }

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d : Calling esp_event_handler_instance_register", __func__,__LINE__);
            status = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler,nullptr,nullptr);
            ESP_LOGI(_log_tag, "%s:%d : esp_event_handler_instance_register:%s", __func__,__LINE__, esp_err_to_name(status));
        }

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d : Calling esp_event_handler_instance_register", __func__,__LINE__);
            status = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler,nullptr,nullptr);
            ESP_LOGI(_log_tag, "%s:%d : esp_event_handler_instance_register:%s", __func__,__LINE__, esp_err_to_name(status));
        }

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d : Calling esp_wifi_set_mode", __func__,__LINE__);
            status = esp_wifi_set_mode(WIFI_MODE_STA);
            ESP_LOGI(_log_tag, "%s:%d : esp_wifi_set_mode:%s", __func__,__LINE__, esp_err_to_name(status));
        }

        if (ESP_OK == status)
        {
            const size_t ssid_length_to_copy {std::min(strlen(ssid),sizeof(wifi_config.sta.ssid))};
            memcpy(wifi_config.sta.ssid,ssid,ssid_length_to_copy);
            if (sizeof(wifi_config.sta.ssid) < strlen(ssid)) {
                status = ESP_FAIL;
            }
            const size_t pwd_length_to_copy {std::min(strlen(password),sizeof(wifi_config.sta.ssid))};

            memcpy(wifi_config.sta.password,password,pwd_length_to_copy);

            wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
            wifi_config.sta.pmf_cfg.capable = true;
            wifi_config.sta.pmf_cfg.required = false;
            
            ESP_LOGI(_log_tag, "%s:%d : Calling esp_wifi_set_config", __func__,__LINE__);
            status = esp_wifi_set_config(WIFI_IF_STA,&wifi_config); // pass by address
            ESP_LOGI(_log_tag, "%s:%d : esp_wifi_set_config:%s", __func__,__LINE__, esp_err_to_name(status));
        }

        if (ESP_OK == status) {
            ESP_LOGI(_log_tag, "%s:%d : Calling esp_wifi_start", __func__,__LINE__);
            status = esp_wifi_start();// I am here, so there is a wifi event
            ESP_LOGI(_log_tag, "%s:%d : esp_wifi_start:%s", __func__,__LINE__, esp_err_to_name(status));
        }

        if (ESP_OK == status) {
            ESP_LOGI(_log_tag, "%s:%d : INITIALISED", __func__,__LINE__);
            _state = state_e::INITIALISED;
        }

    } else if(state_e::ERROR == _state)
    {
        ESP_LOGE(_log_tag, "%s:%d : FAILED", __func__,__LINE__);
        status = ESP_FAIL;
    }

    return status;
}

esp_err_t Wifi::begin(void) 
{
    ESP_LOGI(_log_tag, "%s:%d : Waiting for connect mutex", __func__,__LINE__);
    std::lock_guard<std::mutex> connect_guard(connect_mutex);
    esp_err_t status {ESP_OK};
    ESP_LOGI(_log_tag, "%s:%d : Waiting for state mutex", __func__,__LINE__);
    std::lock_guard<std::mutex> state_guard(state_mutex);

    switch(_state)
    {
        case state_e::READY_TO_CONNECT:
            ESP_LOGI(_log_tag, "%s:%d : Calling esp_wifi_connect", __func__,__LINE__);
            status = esp_wifi_connect();
            ESP_LOGI(_log_tag, "%s:%d : esp_wifi_connect:%s", __func__,__LINE__, esp_err_to_name(status));

            if (ESP_OK == status) {
                _state = state_e::CONNECTING;
            }
            break;
        case state_e::CONNECTING:
        case state_e::WAITING_FOR_IP:
        case state_e::CONNECTED:
            break;
        case state_e::NOT_INITIALISED:
        case state_e::INITIALISED:
        case state_e::DISCONNECTED:
        case state_e::ERROR:
            ESP_LOGE(_log_tag, "%s:%d : Error state", __func__,__LINE__);// bad, pass it a state
            status =  ESP_FAIL;
            break;
    }

    return status;

}
/* get_mac() is a method that uses the Espressif IDF to get the mac address of the chip, and it converts it to ASCII Hex c style string */
esp_err_t Wifi::_get_mac()
{    
    uint8_t mac_byte_buffer[6] {};// A mac is generally 6 bytes.
    // the mac coming from the api is going to be put in this variable

    const esp_err_t status { esp_efuse_mac_get_default(mac_byte_buffer)};// get the mac address from the idf

    if (ESP_OK == status) {
        snprintf(mac_addr_cstr,sizeof(mac_addr_cstr),"%02X%02X%02X%02X%02X%02X",
        mac_byte_buffer[0],
        mac_byte_buffer[1],
        mac_byte_buffer[2],
        mac_byte_buffer[3],
        mac_byte_buffer[4],
        mac_byte_buffer[5]);// format the mac coming from the api accordingly, by making sure every byte is output as a two digit hex.
    }
    return status;
}// not calling this a lot because underlined data is unlikely to change

}// namespace WIFI