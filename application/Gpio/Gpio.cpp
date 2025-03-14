#include "Gpio.h"

namespace Gpio
{
    [[nodiscard]] esp_err_t GpioBase::init(void)
    {
        esp_err_t status {ESP_OK};

        status |= gpio_config(&_cfg);// passing by adress, that's why we have the  &_cfg

        return status;
    }
    [[nodicard]] esp_err_t GpioOutput::init(void)
    {
        esp_err_t status {GpioBase::init()};

        if (ESP_OK == status ) {
            status |= set(_inverted_logic);
        }

        return status;
    }


    // 
    [[nodicard]] bool GpioInput::state(void)
    {
        // esp_err_t status {GpioBase::init()};

        // if (ESP_OK == status ) {
        //     status |= set(_inverted_logic);
        // }

        // return status;
        if (gpio_get_level(_pin) == 0) {
            return false;
        }
        return true;
    }
    // 
    esp_err_t GpioOutput::set(const bool state)
    {
        _state = state;
        return gpio_set_level(_pin,_inverted_logic ? !state : state);
    }
}