set(SOURCES ./Src/main.cpp)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . INC
                    REQUIRES freertos esp_common esp_event esp_timer mqtt esp_http_client esp-tls
                    nvs_flash Gpio Wifi)