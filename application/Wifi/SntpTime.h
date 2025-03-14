#include <ctime>
#include <chrono>
#include <iomanip>
#include <string>

#include "esp_sntp.h"
// #include <lwip/apps/sntp.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Wifi.h"

namespace SNTP {
class Sntp final : private WIFI::Wifi 
{// Sntp inherits from Wifi, all public and protected members of Wifi class become private members in Sntp
    constexpr static const char* _log_tag {"Sntp"}; // since we are inheriting from Wifi, and if we didn't have this variable here, it was going to go and see if wifi has one and we would take the one wifi has. but since we have it here we're gonna use that one for an Sntp object.
    Sntp(void)  = default;
    ~Sntp(void) { esp_sntp_stop(); }; // destructor

    static void callback_on_ntp_update(timeval* tv);

public:// we want a singleton here
    static Sntp& get_instance(void)
    {
        static Sntp sntp;// effectively a singleton because of the static keyword, this is will just eb initialized once. It should be in the static memory, not heap or stack memory
        return sntp;
    }

    enum time_source_e : uint8_t
    {
        TIME_SRC_UNKNOWN    = 0,
        TIME_SRC_NTP        = 1,
        TIME_SRC_GPS        = 2,
        TIME_SRC_RADIO      = 3,
        TIME_SRC_MANUAL     = 4,
        TIME_SRC_ATOMIC_CLK = 5,
        TIME_SRC_CELL_NET   = 6
    };

    static esp_err_t init(void);
    static esp_err_t begin(void);

    // nodiscard means that if someone tries to call this function but doesn't assign the return type to a variable, it's going to trigger a warning
    [[nodiscard]] static const auto time_point_now(void) noexcept
    {
        // noexcept tells the compiler that the function doesn't throw exceptions
        // using auto because otherwise we would have to put std::chrono::_V2::system_clock::time_point. That's absolutely HORIFIC!
        return std::chrono::system_clock::now();
    }

    // delta T since last time we had a time update
    [[nodiscard]] static const auto time_since_last_update(void) noexcept
    {
        return (time_point_now() - last_update);
    }

    [[nodiscard]] static const char* ascii_time_now(void);

    [[nodiscard]] static std::chrono::seconds epoch_seconds(void) noexcept
    {
        return std::chrono::duration_cast<std::chrono::seconds>(time_point_now().time_since_epoch());
    }

private:
    // we can use auto but it may get a bit messy, especially if the compiler doesn't get the type right and we may end up with undefined behavior
    static std::chrono::_V2::system_clock::time_point last_update;// you can't default initialze here if not constexpr
    static time_source_e source;
    static bool _running;
};

} // namespace SNTP

