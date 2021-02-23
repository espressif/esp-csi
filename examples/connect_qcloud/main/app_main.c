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

#include "utils.h"
#include "qcloud_light.h"

#include "ping/ping_sock.h"

#define CONFIG_CSI_BUF_SIZE          32
#define CONFIG_GPIO_LED_MOVE_STATUS  GPIO_NUM_18
#define CSI_SAFTAP_SSID              "qcloud_csi_test"

static const char *TAG  = "app_main";
static wifi_rader_data_t g_wifi_rader_data = {
    .move_relative_threshold = 1.5,
    .move_absolute_threshold = 0.2,
    .status_enable           = true,
};

esp_err_t wifi_rader_get_data(wifi_rader_data_t *data)
{
    *data = g_wifi_rader_data;

    return ESP_OK;
}

esp_err_t wifi_rader_set_data(const wifi_rader_data_t *data)
{
    g_wifi_rader_data = *data;

    return ESP_OK;
}

static void wifi_rader_cb(const wifi_rader_info_t *info, void *ctx)
{
    if (!g_wifi_rader_data.status_enable) {
        return;
    }

    static int s_count = 0;
    static int s_threshold_adjust_count = 0;
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

    if (g_wifi_rader_data.threshold_adjust) {
        if (!s_threshold_adjust_count) {
            g_wifi_rader_data.move_relative_threshold = 0;
            g_wifi_rader_data.move_absolute_threshold = 0;
            light_driver_breath_start(255, 255, 0); /**< yellow blink */
        }

        s_threshold_adjust_count++;

        if (s_threshold_adjust_count == CONFIG_CSI_BUF_SIZE) {
            g_wifi_rader_data.move_relative_threshold = 0;
            g_wifi_rader_data.move_absolute_threshold = 0;
            ESP_LOGW(TAG, "Start adjusting the threshold");
        } else if (s_threshold_adjust_count >= CONFIG_CSI_BUF_SIZE) {
            amplitude_std_avg = trimmean(s_amplitude_std_list, CONFIG_CSI_BUF_SIZE, 0.10);
            g_wifi_rader_data.move_absolute_threshold = max(s_amplitude_std_list, CONFIG_CSI_BUF_SIZE, 0.10);

            if (amplitude_std - amplitude_std_avg > g_wifi_rader_data.move_relative_threshold) {
                g_wifi_rader_data.move_relative_threshold =  amplitude_std - amplitude_std_avg;
            }
        }

        ESP_LOGI(TAG, "threshold_adjust <%d> time: %u ms, rssi: %d, corr: %.3f, std: %.3f, threshold: %.3f/%.3f",
                 s_threshold_adjust_count, info->time_end - info->time_start, info->rssi_avg, amplitude_corr, amplitude_std,
                 g_wifi_rader_data.move_absolute_threshold, g_wifi_rader_data.move_relative_threshold);

        return;
    } else if (s_threshold_adjust_count) {
        ESP_LOGW(TAG, "Stop adjusting the threshold");
        light_driver_breath_stop();
        light_driver_set_switch(false);
        s_threshold_adjust_count = 0;
    }

    if (s_count > CONFIG_CSI_BUF_SIZE) {
        amplitude_std_max = max(s_amplitude_std_list, CONFIG_CSI_BUF_SIZE, 0.10);
        amplitude_std_avg = trimmean(s_amplitude_std_list, CONFIG_CSI_BUF_SIZE, 0.10);

        for (int i = 1, count = 0; i < 6; ++i) {
            if (s_amplitude_std_list[(s_count - i) % CONFIG_CSI_BUF_SIZE] > amplitude_std_avg + g_wifi_rader_data.move_relative_threshold
                    || s_amplitude_std_list[(s_count - i) % CONFIG_CSI_BUF_SIZE] > amplitude_std_max) {
                if (++count > 2) {
                    trigger_relative_flag = true;
                    break;
                }
            }
        }
    }

    static uint32_t s_last_move_time = 0;

    if (amplitude_std > g_wifi_rader_data.move_absolute_threshold || trigger_relative_flag) {
        ESP_LOGW(TAG, "Someone is moving");
        light_driver_set_switch(true);

        g_wifi_rader_data.awake_count++;
        g_wifi_rader_data.room_status = true;

        if (!s_last_move_time) {
            qcloud_light_report_status(NULL);
        }

        s_last_move_time = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);;
    }

    if (s_last_move_time && xTaskGetTickCount() * (1000 / configTICK_RATE_HZ) - s_last_move_time > 3 * 1000) {
        s_last_move_time = 0;
        g_wifi_rader_data.room_status = false;
        light_driver_set_switch(false);
    }

    ESP_LOGI(TAG, "<%d> time: %u ms, rssi: %d, corr: %.3f, std: %.3f, std_avg: %.3f, std_max: %.3f, threshold: %.3f/%.3f, trigger: %d/%d, free_heap: %u/%u",
             s_count, info->time_end - info->time_start, info->rssi_avg,
             amplitude_corr, amplitude_std, amplitude_std_avg, amplitude_std_max,
             g_wifi_rader_data.move_absolute_threshold, g_wifi_rader_data.move_relative_threshold,
             amplitude_std > g_wifi_rader_data.move_absolute_threshold, trigger_relative_flag,
             esp_get_minimum_free_heap_size(), esp_get_free_heap_size());
}

esp_err_t esp_wifi_rader_ping_start()
{
    static esp_ping_handle_t ping_handle = NULL;
    esp_ping_config_t ping_config = {
        .count = 0,
        .interval_ms = 10,
        .timeout_ms = 1000,
        .data_size = 1,
        .tos = 0,
        .task_stack_size = 4096,
        .task_prio = 0,
    };

    esp_netif_ip_info_t local_ip;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &local_ip);
    ESP_LOGI(TAG, "got ip:" IPSTR ", gw: " IPSTR, IP2STR(&local_ip.ip), IP2STR(&local_ip.gw));
    inet_addr_to_ip4addr(ip_2_ip4(&ping_config.target_addr), (struct in_addr *)&local_ip.gw);

    esp_ping_callbacks_t cbs = {0};
    esp_ping_new_session(&ping_config, &cbs, &ping_handle);
    esp_ping_start(ping_handle);

    return ESP_OK;
}

void app_main(void)
{
    wifi_rader_config_t rader_config = {
        .wifi_rader_cb = wifi_rader_cb,
        .filter_len = 384,
    };

    /**
     * @brief Initialize Wi-Fi radar
     */
    qcloud_light_init();

    if (esp_qcloud_storage_get("move_absolute", &g_wifi_rader_data.move_absolute_threshold, sizeof(float)) != ESP_OK) {
        g_wifi_rader_data.move_absolute_threshold = 0.2;
    }

    if (esp_qcloud_storage_get("move_relative", &g_wifi_rader_data.move_relative_threshold, sizeof(float)) != ESP_OK) {
        g_wifi_rader_data.move_relative_threshold = 1.5;
    }

    if (esp_qcloud_storage_get("filter_mac", rader_config.filter_mac,
                               sizeof(rader_config.filter_mac)) != ESP_OK) {
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        memcpy(rader_config.filter_mac, ap_info.bssid, sizeof(ap_info.bssid));
        ESP_LOGI(TAG, "Using router as the source of csi data");
    } else {
        ESP_LOGI(TAG, "Using " MACSTR " as the source of csi data", MAC2STR(rader_config.filter_mac));
    }

    esp_wifi_rader_ping_start();

    esp_wifi_rader_init();
    esp_wifi_rader_set_config(&rader_config);
    esp_wifi_rader_start();

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CSI_SAFTAP_SSID,
            .ssid_len = strlen(CSI_SAFTAP_SSID),
            .max_connection = 3,
        },
    };

    if(!esp_netif_get_handle_from_ifkey("WIFI_AP_DEF")) {
        esp_netif_create_default_wifi_ap();
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
}
