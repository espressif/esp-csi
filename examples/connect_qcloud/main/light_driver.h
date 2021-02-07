// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "led_pwm.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @brief The mode of the five-color light
 */
enum light_mode {
    MODE_NONE                = 0,
    MODE_RGB                 = 1,
    MODE_HSV                 = 2,
    MODE_CTB                 = 3,
    MODE_ON                  = 4,
    MODE_OFF                 = 5,
    MODE_HUE_INCREASE        = 4,
    MODE_HUE_DECREASE        = 5,
    MODE_WARM_INCREASE       = 6,
    MODE_WARM_DECREASE       = 7,
    MODE_BRIGHTNESS_INCREASE = 8,
    MODE_BRIGHTNESS_DECREASE = 9,
};

/**
 * @brief Light driven configuration
 */
typedef struct {
    char *type;               /**< Type of development board */
    gpio_num_t gpio_red;      /**< Red corresponds to GPIO */
    gpio_num_t gpio_green;    /**< Green corresponds to GPIO */
    gpio_num_t gpio_blue;     /**< Blue corresponds to GPIO */
    gpio_num_t gpio_cold;     /**< Cool corresponds to GPIO */
    gpio_num_t gpio_warm;     /**< Warm corresponds to GPIO */
    uint32_t fade_period_ms;  /**< The time from the current color to the next color */
    uint32_t blink_period_ms; /**< Period of flashing lights */
} light_driver_config_t;

#ifdef CONFIG_LIGHT_TYPE_MESHKIT
#define COFNIG_LIGHT_TYPE_DEFAULT() { \
        .type            = "meshlight",\
                           .gpio_red        = GPIO_NUM_4,\
                                              .gpio_green      = GPIO_NUM_16,\
                                                      .gpio_blue       = GPIO_NUM_5,\
                                                              .gpio_cold       = GPIO_NUM_23,\
                                                                      .gpio_warm       = GPIO_NUM_19,\
                                                                              .fade_period_ms  = CONFIG_LIGHT_FADE_PERIOD_MS,\
                                                                                      .blink_period_ms = CONFIG_LIGHT_BLINK_PERIOD_MS,\
    }
#elif CONFIG_LIGHT_TYPE_MOONLIGHT
#define COFNIG_LIGHT_TYPE_DEFAULT() { \
        .type            = "moonlight",\
                           .gpio_red        = GPIO_NUM_16,\
                                              .gpio_green      = GPIO_NUM_4,\
                                                      .gpio_blue       = GPIO_NUM_17,\
                                                              .fade_period_ms  = CONFIG_LIGHT_FADE_PERIOD_MS,\
                                                                      .blink_period_ms = CONFIG_LIGHT_BLINK_PERIOD_MS,\
    }
#else
#define COFNIG_LIGHT_TYPE_DEFAULT() {\
        .type            = "light",\
                           .gpio_red        = 1,\
                                              .gpio_green      = 2,\
                                                      .gpio_blue       = 3,\
                                                              .gpio_cold       = 4,\
                                                                      .gpio_warm       = 5,\
                                                                              .fade_period_ms  = CONFIG_LIGHT_FADE_PERIOD_MS,\
                                                                                      .blink_period_ms = CONFIG_LIGHT_BLINK_PERIOD_MS,\
    }
#endif /**< LIGHT_TYPE_DEFAULT */

/**
 * @brief Light initialize.
 *
 * @param[in] config [description]
 *
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_init(light_driver_config_t *config);

/**
 * @brief Light deinitialize.
 *
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_deinit(void);

/**
 * @brief Set the fade time of the light.
 *
 * @param[in] fade_period_ms  The time from the current color to the next color.
 * @param[in] blink_period_ms Light flashing frequency.
 *
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_config(uint32_t fade_period_ms, uint32_t blink_period_ms);

/**
 * @brief Set the hue of the light.
 *
 * @param[in] hue Value ranges from 0 to 360.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_set_hue(uint16_t hue);

/**
 * @brief Set the saturation of the light.
 *
 * @param[in] saturation Value ranges from 0 to 100.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_set_saturation(uint8_t saturation);

/**
 * @brief Set the saturation of the light.
 *
 * @param[in] value Value ranges from 0 to 100.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_set_value(uint8_t value);

/**
 * @brief Set the color temperature of the light.
 *
 * @param[in] color_temperature Value ranges from 0 to 100.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_set_color_temperature(uint8_t color_temperature);

/**
 * @brief Set the color brightness of the light.
 *
 * @param[in] brightness Value ranges from 0 to 100.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_set_brightness(uint8_t brightness);

/**
 * @brief Use HSV to set the light.
 *
 * @param[in] hue Value ranges from 0 to 360.
 * @param[in] saturation Value ranges from 0 to 100.
 * @param[in] value Value ranges from 0 to 100.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_set_hsv(uint16_t hue, uint8_t saturation, uint8_t value);

/**
 * @brief Use CTB to set up lights.
 *
 * @param[in] color_temperature Value ranges from 0 to 100.
 * @param[in] brightness Value ranges from 0 to 100.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_set_ctb(uint8_t color_temperature, uint8_t brightness);

/**
 * @brief Set the state of the light
 *
 * @param[in] status On or off
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_set_switch(bool status);

/**
 * @brief Get the hue of the light.
 *
 * @return Current hue.
 */
uint16_t light_driver_get_hue(void);

/**
 * @brief Get the saturation of the light.
 *
 * @return Current saturation.
 */
uint8_t light_driver_get_saturation(void);

/**
 * @brief Get the value of the light.
 *
 * @return Current value.
 */
uint8_t light_driver_get_value(void);

/**
 * @brief Get the HSV of the light.
 *
 * @param[in/out] hue This variable is used to store hue.
 * @param[in/out] saturation This variable is used to store saturation.
 * @param[in/out] value This variable is used to store value.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_get_hsv(uint16_t *hue, uint8_t *saturation, uint8_t *value);

/**
 * @brief Get the color temperature of the light.
 *
 * @return Current color temperature.
 */
uint8_t light_driver_get_color_temperature(void);

/**
 * @brief Get the brightness of the light.
 *
 * @return Current brightness.
 */
uint8_t light_driver_get_brightness(void);

/**
 * @brief Get the CTB of the light.
 *
 * @param[in/out] color_temperature This variable is used to store color temperature.
 * @param[in/out] brightness This variable is used to store brightness.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_get_ctb(uint8_t *color_temperature, uint8_t *brightness);

/**
 * @brief Get the state of the light.
 *
 * @return true Turn on the light.
 * @return false Turn off the light.
 */
bool light_driver_get_switch(void);

/**
 * @brief Get the mode of the light.
 *
 * @return Current mode.
 */
uint8_t light_driver_get_mode(void);

/**
 * @brief Get the type of the light.
 *
 * @return Pointer to type.
 */
char *light_driver_get_type(void);


/**
 * @brief  Used to indicate the operating mode, such as configuring the network mode, upgrading mode.
 *
 * @note   The state of the light is not saved in nvs.
 * @return
 *      - ESP_OK
 *      - ESP_QCLOUD_ERR_INVALID_ARG
 */
esp_err_t light_driver_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Turn on breathing light mode.
 *
 * @param[] red Value ranges from 0 to 255.
 * @param[] green Value ranges from 0 to 255.
 * @param[] blue  Value ranges from 0 to 255.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_breath_start(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Turn off breathing light mode.
 *
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_breath_stop(void);

/**
 * @brief Brightness gradient.
 *
 * @param[in] brightness Value ranges from 0 to 100.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_fade_brightness(uint8_t brightness);

/**
 * @brief Hue gradient
 *
 * @param[in] hue Value ranges from 0 to 360.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_fade_hue(uint16_t hue);

/**
 * @brief Color temperature gradient
 *
 * @param[in] color_temperature Value ranges from 0 to 100.
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_fade_warm(uint8_t color_temperature);

/**
 * @brief Stop fade function.
 *
 * @return
 *      - ESP_OK
 *      - ESP_FAIL
 */
esp_err_t light_driver_fade_stop(void);

#ifdef __cplusplus
}
#endif
