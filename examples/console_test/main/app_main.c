/* Wi-Fi CSI Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "esp_console.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_rader.h"
#include "app_priv.h"

#define CONFIG_CSI_BUF_SIZE          50
#define CONFIG_GPIO_LED_MOVE_STATUS  GPIO_NUM_18

static const char *TAG  = "app_main";

float g_move_absolute_threshold = 0.3;
float g_move_relative_threshold = 1.5;

static void wifi_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_csi_raw_cb(void *ctx, wifi_csi_info_t *info)
{
    wifi_pkt_rx_ctrl_t *rx_ctrl = &info->rx_ctrl;
    static uint32_t s_count = 0;

    if (!s_count) {
        ets_printf("type,id,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,len,first_word,data\n");
    }

    ets_printf("CSI_DATA,%d," MACSTR ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
               s_count++, MAC2STR(info->mac), rx_ctrl->rssi, rx_ctrl->rate, rx_ctrl->sig_mode,
               rx_ctrl->mcs, rx_ctrl->cwb, rx_ctrl->smoothing, rx_ctrl->not_sounding,
               rx_ctrl->aggregation, rx_ctrl->stbc, rx_ctrl->fec_coding, rx_ctrl->sgi,
               rx_ctrl->noise_floor, rx_ctrl->ampdu_cnt, rx_ctrl->channel, rx_ctrl->secondary_channel,
               rx_ctrl->timestamp, rx_ctrl->ant, rx_ctrl->sig_len, rx_ctrl->rx_state);

    ets_printf(",%d,%d,[ ", info->len, info->first_word_invalid);

    for (int i = 0; i < info->len; i++) {
        ets_printf("%d ", info->buf[i]);
    }

    ets_printf("]\n");
}


static void wifi_rader_cb(const wifi_rader_info_t *info, void *ctx)
{
    static int s_count = 0;
    wifi_rader_config_t rader_config = {0};
    static float s_amplitude_std_list[CONFIG_CSI_BUF_SIZE];
    bool trigger_relative_flag = false;

    esp_wifi_rader_get_config(&rader_config);

    float amplitude_std  = avg(info->amplitude_std, rader_config.filter_len / 128);
    float amplitude_corr = avg(info->amplitude_corr, rader_config.filter_len / 128);
    float amplitude_std_max = 0;
    float amplitude_std_avg = 0;
    s_amplitude_std_list[s_count % CONFIG_CSI_BUF_SIZE] = amplitude_std;

    s_count++;

    if (s_count > CONFIG_CSI_BUF_SIZE) {
        amplitude_std_max = max(s_amplitude_std_list, CONFIG_CSI_BUF_SIZE, 0.10);
        amplitude_std_avg = trimmean(s_amplitude_std_list, CONFIG_CSI_BUF_SIZE, 0.10);

        for (int i = 1, count = 0; i < 6; ++i) {
            if (s_amplitude_std_list[(s_count - i) % CONFIG_CSI_BUF_SIZE] > amplitude_std_avg * g_move_relative_threshold
                    || s_amplitude_std_list[(s_count - i) % CONFIG_CSI_BUF_SIZE] > amplitude_std_max) {
                if (++count > 2) {
                    trigger_relative_flag = true;
                    break;
                }
            }
        }
    }

    static uint32_t s_last_move_time = 0;

    if (amplitude_std > g_move_absolute_threshold || trigger_relative_flag) {
        gpio_set_level(CONFIG_GPIO_LED_MOVE_STATUS, 1);
        s_last_move_time = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);;
        ESP_LOGW(TAG, "Someone is moving");
    }

    if (s_last_move_time && xTaskGetTickCount() * (1000 / configTICK_RATE_HZ) - s_last_move_time > 3 * 1000){
        s_last_move_time  = 0;
        gpio_set_level(CONFIG_GPIO_LED_MOVE_STATUS, 0);
    }

    ESP_LOGI(TAG, "<%d> time: %u ms, rssi: %d, corr: %.3f, std: %.3f, std_avg: %.3f, std_max: %.3f, threshold: %.3f/%.3f, trigger: %d/%d, free_heap: %u/%u",
             s_count, info->time_end - info->time_start, info->rssi_avg,
             amplitude_corr, amplitude_std, amplitude_std_avg, amplitude_std_max,
             g_move_absolute_threshold, g_move_relative_threshold,
             amplitude_std > g_move_absolute_threshold, trigger_relative_flag,
             esp_get_minimum_free_heap_size(), esp_get_free_heap_size());
}

void app_main(void)
{
    wifi_rader_config_t rader_config = {
        .wifi_rader_cb = wifi_rader_cb,
        // .filter_len = 384,
        // .filter_mac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
        // .wifi_csi_raw_cb = wifi_csi_raw_cb,
    };

    /**
     * @brief Used to show whether someone is moving
     */
    gpio_reset_pin(CONFIG_GPIO_LED_MOVE_STATUS);
    gpio_set_direction(CONFIG_GPIO_LED_MOVE_STATUS, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_GPIO_LED_MOVE_STATUS, 0);

    /**
     * @brief Initialize Wi-Fi radar
     */
    wifi_init();
    esp_wifi_rader_init();
    esp_wifi_rader_set_config(&rader_config);
    esp_wifi_rader_start();

    /**
     * @brief Register serial command
     */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    repl_config.prompt = "csi>";
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    cmd_register_system();
    cmd_register_wifi();
    cmd_register_ping();
    cmd_register_csi();
    cmd_register_detect();

    printf("\n");
    printf(" =======================================================================\n");
    printf(" |                    Steps to test CSI                                |\n");
    printf(" |                                                                     |\n");
    printf(" |  1. Print 'help' to gain overview of commands                       |\n");
    printf(" |     "LOG_COLOR_I"csi>"LOG_RESET_COLOR" help                                                       |\n");
    printf(" |  2. Start SoftAP or Sta on another ESP32/ESP32S2/ESP32C3            |\n");
    printf(" |     "LOG_COLOR_I"csi>"LOG_RESET_COLOR" sta <ssid> <pwd>                                           |\n");
    printf(" |     "LOG_COLOR_I"csi>"LOG_RESET_COLOR" ap <ssid> <pwd>                                            |\n");
    printf(" |  3. Run ping to test                                                |\n");
    printf(" |     "LOG_COLOR_I"csi>"LOG_RESET_COLOR" ping <ip> -c <count> -i <interval>                         |\n");
    printf(" |  4. Configure CSI parameters                                        |\n");
    printf(" |     "LOG_COLOR_I"csi>"LOG_RESET_COLOR" csi -m <mac(xx:xx:xx:xx:xx:xx)> -l <len>                   |\n");
    printf(" |  5. Configure CSI parameters                                        |\n");
    printf(" |     "LOG_COLOR_I"csi>"LOG_RESET_COLOR" detect -a <absolute_threshold> -r <relative_threshold>     |\n");
    printf(" |                                                                     |\n");
    printf(" ======================================================================\n\n");

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
