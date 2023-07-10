/*  WS2812 RGB LED helper functions

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* It is recommended to copy this code in your example so that you can modify as
 * per your application's needs.
 */


#include <stdint.h>
#include <esp_err.h>
#include <esp_log.h>

static const char *TAG = "ws2812_led";

#ifdef CONFIG_WS2812_LED_ENABLE
#include <driver/rmt.h>
#include "led_strip.h"
#define RMT_TX_CHANNEL RMT_CHANNEL_0

static led_strip_t *g_strip;

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
static void ws2812_led_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

esp_err_t ws2812_led_set_rgb(uint32_t red, uint32_t green, uint32_t blue)
{
    if (!g_strip) {
        return ESP_ERR_INVALID_STATE;
    }
    g_strip->set_pixel(g_strip, 0, red, green, blue);
    g_strip->refresh(g_strip, 100);
    return ESP_OK;
}

esp_err_t ws2812_led_set_hsv(uint32_t hue, uint32_t saturation, uint32_t value)
{
    if (!g_strip) {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    ws2812_led_hsv2rgb(hue, saturation, value, &red, &green, &blue);
    return ws2812_led_set_rgb(red, green, blue);
}

esp_err_t ws2812_led_clear(void)
{
    if (!g_strip) {
        return ESP_ERR_INVALID_STATE;
    }
    g_strip->clear(g_strip, 100);
    return ESP_OK;
}

esp_err_t ws2812_led_init(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_WS2812_LED_GPIO, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    if (rmt_config(&config) != ESP_OK) {
        ESP_LOGE(TAG, "RMT Config failed.");
        return ESP_FAIL;
    }
    if (rmt_driver_install(config.channel, 0, 0) != ESP_OK) {
        ESP_LOGE(TAG, "RMT Driver install failed.");
        return ESP_FAIL;
    }

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(1, (led_strip_dev_t)config.channel);
    g_strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!g_strip) {
        ESP_LOGE(TAG, "Install WS2812 driver failed.");
        return ESP_FAIL;
    }
    return ESP_OK;
}
#else /* !CONFIG_WS2812_LED_ENABLE */
esp_err_t ws2812_led_set_rgb(uint32_t red, uint32_t green, uint32_t blue)
{
    /* Empty function, since WS2812 RGB LED has been disabled.  */
    return ESP_OK;
}

esp_err_t ws2812_led_set_hsv(uint32_t hue, uint32_t saturation, uint32_t value)
{
    /* Empty function, since WS2812 RGB LED has been disabled.  */
    return ESP_OK;
}

esp_err_t ws2812_led_clear(void)
{
    /* Empty function, since WS2812 RGB LED has been disabled.  */
    return ESP_OK;
}

esp_err_t ws2812_led_init(void)
{
    ESP_LOGW(TAG, "WS2812 LED is disabled");
    return ESP_OK;
}
#endif /* !CONFIG_WS2812_LED_ENABLE */
