#pragma once

#include "driver/gpio.h"

namespace Gpio
{

    class GpioBase
    {// abstract base class, because different behaviors are gonna be present
    // private are implicit
    protected:
        const gpio_num_t _pin;
        const bool _inverted_logic = false;
        const gpio_config_t _cfg;
        public:
            constexpr GpioBase(const gpio_num_t pin,const gpio_config_t& config,const bool _invert_logic = false):
                _pin {pin},
                _inverted_logic {_invert_logic},
                _cfg {config}// because config is const, here we trigger copy constructor, if it wasn't const, it would trigger the move constructor, and thanks to constexpr in the constructor, this would resolve at compile time 
            {}
            virtual bool state(void) = 0;// meaning this method has to be defined in any derived class
            virtual esp_err_t set(const bool state) = 0;
            [[nodiscard]] esp_err_t init(void);// same for input/output
    };//Gpio Base

    class GpioOutput:GpioBase
    {// like a screen
        // pullup / pulldown??
        bool _state = false;    // class initialization, maps the user wish
        // const bool _inverted_logic = false;
        //const gpio_num_t _pin;  // because we need to know the pin number in the whole class
        public:
            constexpr GpioOutput(const gpio_num_t pin, const bool invert = false) :
                GpioBase{pin,
                gpio_config_t {
                    .pin_bit_mask = static_cast<uint64_t>(1) << pin,
                    .mode         = GPIO_MODE_OUTPUT,
                    .pull_up_en   = GPIO_PULLUP_DISABLE,
                    .pull_down_en = GPIO_PULLDOWN_ENABLE,
                    .intr_type    = GPIO_INTR_DISABLE
                },
                invert}
                // _inverted_logic {invert},
                // _pin {pin}
            {

            }
            esp_err_t init();// very likely it's not gonna stay as void because we wanna know which pin it's operating on
            esp_err_t set(const bool state);
            //esp_err_t toggle(void);
            bool state(void) {
                return _state;
            }
    };// Gpio Output

    class GpioInput : GpioBase 
    {// General purpose input output, like a button
        // no need for a setter because we are not going to be setting the pin
        //gpio_num_t _pin;
        //const bool _inverted_logic = false;
        public:
            constexpr GpioInput(const gpio_num_t pin, const bool invert = false):
                // _pin {pin},
                GpioBase{pin,
                        gpio_config_t {
                            .pin_bit_mask = static_cast<uint64_t>(1) << pin,
                            .mode         = GPIO_MODE_INPUT,
                            .pull_up_en   = GPIO_PULLUP_DISABLE,
                            .pull_down_en = GPIO_PULLDOWN_ENABLE,
                            .intr_type    = GPIO_INTR_DISABLE
                        },
                        invert
                }
            {}            
            //esp_err_t init(void);
            esp_err_t set(const bool state) {
                return ESP_OK;
            };
            esp_err_t init() {
                return ESP_OK;
            }
            bool state(void);// current state of a specific pin; pin driven high or low, whether it's driver by a btn or whatever.
    };

}// namespace GPIO



