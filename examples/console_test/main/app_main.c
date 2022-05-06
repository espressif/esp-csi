/* Wi-Fi CSI console Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_console.h"

#include "esp_wifi.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "driver/gpio.h"
#include "driver/rmt.h"
#include "led_strip.h"

#include "esp_radar.h"
#include "csi_commands.h"

#ifdef CONFIG_IDF_TARGET_ESP32C3
#define CONFIG_LED_STRIP_GPIO        GPIO_NUM_8
#elif CONFIG_IDF_TARGET_ESP32S3
#define CONFIG_LED_STRIP_GPIO        GPIO_NUM_48
#else
#define CONFIG_LED_STRIP_GPIO        GPIO_NUM_18
#endif

#define CONFIG_LESS_INTERFERENCE_CHANNEL    11

static float g_corr_threshold = 1.10;
static uint8_t g_action_value = 0;
static uint8_t g_action_id    = 0;

static led_strip_t *g_strip_handle = NULL;
static xQueueHandle g_csi_info_queue = NULL;

static const char *TAG  = "app_main";

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
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_LESS_INTERFERENCE_CHANNEL, WIFI_SECOND_CHAN_BELOW));
}

static struct {
    struct arg_str *corr_threshold;
    struct arg_int *action;
    struct arg_int *mode;
    struct arg_int *id;
    struct arg_end *end;
} radar_args;

static int wifi_cmd_radar(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &radar_args) != ESP_OK) {
        arg_print_errors(stderr, radar_args.end, argv[0]);
        return ESP_FAIL;
    }

    if (radar_args.corr_threshold->count) {
        float threshold = atof(radar_args.corr_threshold->sval[0]);

        if (threshold >= 1.0 && threshold <= 2.0) {
            g_corr_threshold = threshold;
        } else {
            ESP_LOGE(TAG, "If the setting fails, the absolute threshold range of std is: 0.0 ~ 2.0");
        }
    }

    if (radar_args.action->count) {
        g_action_value = radar_args.action->ival[0];
    }

    if (radar_args.id->count) {
        g_action_id = radar_args.id->ival[0];
    }

    if (radar_args.mode->count && radar_args.mode->ival[0]) {
        if (g_action_value) {
            esp_radar_action_calibrate_start(RADAR_ACTION_STATIC);
        } else {
            esp_radar_action_calibrate_stop(RADAR_ACTION_STATIC);
        }
    }

    ESP_LOGI(TAG, "value: %d, id: action %d, threshold: %.2f",
             g_action_value, g_action_id, g_corr_threshold);

    return ESP_OK;
}

void cmd_register_radar(void)
{
    radar_args.corr_threshold = arg_str0("t", "corr_threshold", "<1.0 ~ 2.0>", "Set the corr threshold of std");
    radar_args.mode           = arg_int0("m", "mode", "0 ~ 1 (label: 0, calibrate: 1)", "Mode selection");
    radar_args.action         = arg_int0("a", "actions", "<0 ~ 20>", "CSI data with actions");
    radar_args.id             = arg_int0("i", "id", "id", "CSI data with actions id");
    radar_args.end = arg_end(4);

    const esp_console_cmd_t radar_cmd = {
        .command = "radar",
        .help = "Radar config",
        .hint = NULL,
        .func = &wifi_cmd_radar,
        .argtable = &radar_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&radar_cmd));
}

static void csi_data_print_task(void *arg)
{
    wifi_csi_filtered_info_t *info = NULL;
    static char buffer[8196];

    while (xQueueReceive(g_csi_info_queue, &info, portMAX_DELAY)) {
        wifi_pkt_rx_ctrl_t *rx_ctrl = &info->rx_ctrl;
        static uint32_t s_count = 0;
        size_t len = 0;

        if (!s_count) {
            ESP_LOGI(TAG, "================ CSI RECV ================");
            len += sprintf(buffer + len, "type,id,action_id,action,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,len,first_word,data\n");
        }

        len += sprintf(buffer + len, "CSI_DATA,%d,%d,%d," MACSTR ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%d,%d,%d",
                       s_count++, g_action_id, g_action_value,  MAC2STR(info->mac), rx_ctrl->rssi, rx_ctrl->rate, rx_ctrl->sig_mode,
                       rx_ctrl->mcs, rx_ctrl->cwb, rx_ctrl->smoothing, rx_ctrl->not_sounding,
                       rx_ctrl->aggregation, rx_ctrl->stbc, rx_ctrl->fec_coding, rx_ctrl->sgi,
                       rx_ctrl->noise_floor, rx_ctrl->ampdu_cnt, rx_ctrl->channel, rx_ctrl->secondary_channel,
                       rx_ctrl->timestamp, rx_ctrl->ant, rx_ctrl->sig_len, rx_ctrl->rx_state);

        len += sprintf(buffer + len, ",%d,%d,\"[%d", info->valid_len, 0, info->valid_data[0]);

        for (int i = 1; i < info->valid_len; i++) {
            len += sprintf(buffer + len, ",%d", info->valid_data[i]);
        }

        len += sprintf(buffer + len, "]\"\n");
        printf("%s", buffer);
        free(info);
    }

    vTaskDelete(NULL);
}

void wifi_csi_raw_cb(const wifi_csi_filtered_info_t *info, void *ctx)
{
    wifi_csi_filtered_info_t *q_data = malloc(sizeof(wifi_csi_filtered_info_t) + info->valid_len);
    *q_data = *info;
    memcpy(q_data->valid_data, info->valid_data, info->valid_len);

    if (!g_csi_info_queue || xQueueSend(g_csi_info_queue, &q_data, 0) == pdFALSE) {
        free(q_data);
    }
}

static void wifi_radar_cb(const wifi_radar_info_t *info, void *ctx)
{
    static uint32_t s_count = 0;
    static uint32_t s_trigger_move_corr_count = 0;
    static uint32_t s_last_move_time = 0;
    static float s_corr_last = 0;
    bool trigger_move_corr_flag = 0;

    float corr_diff = (info->amplitude_corr_max - info->amplitude_corr_min) * g_corr_threshold;

    if (corr_diff < 0){
        corr_diff = 1;
    }

    if (fabs(info->amplitude_corr - s_corr_last) > corr_diff) {
        s_trigger_move_corr_count++;
    } else {
        s_trigger_move_corr_count = 0;
    }

    if (s_trigger_move_corr_count > 2) {
        trigger_move_corr_flag = true;
        g_strip_handle->set_pixel(g_strip_handle, 0, 0, 0, 255);
        g_strip_handle->refresh(g_strip_handle, 100);

        s_last_move_time = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
        ESP_LOGW(TAG, "Someone moved");
    }

    if (s_last_move_time && xTaskGetTickCount() * (1000 / configTICK_RATE_HZ) - s_last_move_time > 3 * 1000) {
        s_last_move_time  = 0;
        g_strip_handle->clear(g_strip_handle, 100);
    }

    // ESP_LOGW(TAG, "s_count: %d, time_spent: %d, std: %f, corr: %f, corr_estimate: %f, min_std: %f, avg_std: %f",
    //          s_count, info->time_spent, info->amplitude_std, info->amplitude_corr, info->amplitude_corr_estimate, amplitude_std_min, amplitude_std_avg);

    if (!s_count) {
        ESP_LOGI(TAG, "================ RADAR RECV ================");
        printf("type,id,corr,corr_max,corr_min,corr_threshold,corr_trigger\n");
    }

    printf("RADAR_DADA,%d,%f,%f,%f,%f,%d\n", 
            s_count++, info->amplitude_corr, s_corr_last + corr_diff, s_corr_last - corr_diff, 
            g_corr_threshold, trigger_move_corr_flag);

    s_corr_last = info->amplitude_corr;
}

static bool g_get_ip_flag = false;

/* Event handler for catching system events */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        g_get_ip_flag = true;
        wifi_radar_config_t radar_config = {0};
        wifi_ap_record_t ap_info;
    
        esp_wifi_sta_get_ap_info(&ap_info); 
        esp_wifi_radar_get_config(&radar_config);
        memcpy(radar_config.filter_mac, ap_info.bssid, sizeof(ap_info.bssid));
        esp_wifi_radar_set_config(&radar_config);
    }
}

void app_main(void)
{
    /**
     * @brief Install ws2812 driver
     */
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_LED_STRIP_GPIO, RMT_CHANNEL_0);
    // set counter clock to 40MHz
    config.clk_div = 2;
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(1, (led_strip_dev_t)config.channel);
    g_strip_handle = led_strip_new_rmt_ws2812(&strip_config);
    g_strip_handle->set_pixel(g_strip_handle, 0, 255, 255, 255);
    ESP_ERROR_CHECK(g_strip_handle->refresh(g_strip_handle, 100));

    /**
     * @brief Initialize Wi-Fi radar
     */
    wifi_init();
    esp_wifi_radar_init();

    /**
     * @brief Register serial command
     */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    repl_config.prompt = "csi>";
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    cmd_register_system();
    cmd_register_ping();
    cmd_register_wifi_config();
    cmd_register_wifi_scan();
    cmd_register_radar();

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    g_csi_info_queue = xQueueCreate(10, sizeof(void *));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_radar_config_t radar_config = {
        .wifi_radar_cb        = wifi_radar_cb,
        .wifi_csi_filtered_cb = wifi_csi_raw_cb,
        .filter_mac           = {0x1a, 0x00, 0x00, 0x00, 0x00, 0x00},
    };

    esp_wifi_radar_set_config(&radar_config);
    esp_wifi_radar_start();

    xTaskCreate(csi_data_print_task, "csi_data_print", 4 * 1024, NULL, 0, NULL);
}
