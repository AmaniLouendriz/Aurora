#include "SntpTime.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

namespace SNTP {
std::chrono::_V2::system_clock::time_point last_update {};
Sntp::time_source_e Sntp::source {Sntp::time_source_e::TIME_SRC_UNKNOWN};
bool Sntp::_running {false};

// constructor



void Sntp::callback_on_ntp_update(timeval* tv)
{
    ESP_LOGI(_log_tag, "Do we ever come here");
    ESP_LOGI(_log_tag, "Time is %s",ascii_time_now());

}

// init
esp_err_t Sntp::init(void)
{
    if (!_running) {
        ESP_LOGI(_log_tag, "%s:%d We are here", __func__,__LINE__);
        // we are calling the init method that we have inherited from Wifi
            // waiting until we got an ip address
            while (state_e::CONNECTED != Wifi::get_state())
            {
                vTaskDelay(pdMS_TO_TICKS(1000));// maybe we can block on a mutex??
            }

            // Now we are connected and we got an ip
            // std offset dst [offset],start[/time],end[/time]
            setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 1);
            tzset();

            esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);

            // we can have multiple servers, which is good, because if one is down, we use the other one.
            esp_sntp_setservername(0, "time.google.com");
            esp_sntp_setservername(1, "pool.ntp.org");

            // each time there is an update, it will call a callback specified by the user
            sntp_set_time_sync_notification_cb(&callback_on_ntp_update);// we can't get a function pointer with an object method, should be static

            sntp_set_sync_interval(/*15**/60*1000);// poll every 15 minutes

            esp_sntp_init();

            source = TIME_SRC_NTP;
            _running = true;
            ESP_LOGI(_log_tag, "%s:%d We are in end", __func__,__LINE__);
    }

    return {_running ? ESP_OK : ESP_FAIL};
}




// ascii_time_now
[[nodiscard]] const char* Sntp::ascii_time_now(void)
{
    const std::time_t time_now {std::chrono::system_clock::to_time_t(time_point_now())};// now we have the current time as a time_t structure

    return std::asctime(std::localtime(&time_now));

}

}// namespace SNTP
