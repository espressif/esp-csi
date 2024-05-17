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
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_console.h"

#include "esp_mac.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "ping/ping_sock.h"
#include "hal/uart_ll.h"
#include "mbedtls/base64.h"

#include "ws2812_led.h"
#include "esp_radar.h"
#include "csi_commands.h"

#define RECV_ESPNOW_CSI
#define CONFIG_LESS_INTERFERENCE_CHANNEL    11
#define CONFIG_SEND_DATA_FREQUENCY          100

#define RADAR_EVALUATE_SERVER_PORT          3232
#define RADAR_BUFF_MAX_LEN                  25

static QueueHandle_t g_csi_info_queue    = NULL;
static bool g_wifi_connect_status        = false;
static uint32_t g_send_data_interval     = 1000 / CONFIG_SEND_DATA_FREQUENCY;
static const char *TAG                   = "app_main";

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
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G));
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_LESS_INTERFERENCE_CHANNEL, WIFI_SECOND_CHAN_BELOW));

#ifdef RECV_ESPNOW_CSI 
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
#endif
}

static struct {
    struct arg_lit *train_start;
    struct arg_lit *train_stop;
    struct arg_lit *train_add;
    struct arg_str *predict_someone_threshold;
    struct arg_str *predict_someone_sensitivity;
    struct arg_str *predict_move_threshold;
    struct arg_str *predict_move_sensitivity;
    struct arg_int *predict_buff_size;
    struct arg_int *predict_outliers_number;
    struct arg_str *collect_taget;
    struct arg_int *collect_number;
    struct arg_int *collect_duration;
    struct arg_lit *csi_start;
    struct arg_lit *csi_stop;
    struct arg_str *csi_output_type;
    struct arg_str *csi_output_format;
    struct arg_int *csi_scale_shift;
    struct arg_int *channel_filter;
    struct arg_int *send_data_interval;
    struct arg_end *end;
} radar_args;
static struct console_input_config {
    bool train_start;
    float predict_someone_threshold;
    float predict_someone_sensitivity;
    float predict_move_threshold;
    float predict_move_sensitivity;
    uint32_t predict_buff_size;
    uint32_t predict_outliers_number;
    char collect_taget[16];
    uint32_t collect_number;
    char csi_output_type[16];
    char csi_output_format[16];
} g_console_input_config = {
    .predict_someone_threshold = 0,
    .predict_someone_sensitivity = 0.15,
    .predict_move_threshold    = 0.0003,
    .predict_move_sensitivity  = 0.20,
    .predict_buff_size         = 5,
    .predict_outliers_number   = 2,
    .train_start               = false,
    .collect_taget             = "unknown",
    .csi_output_type           = "LLFT",
    .csi_output_format         = "decimal"
};

static TimerHandle_t g_collect_timer_handele = NULL;

static void collect_timercb(TimerHandle_t timer)
{
    g_console_input_config.collect_number--;

    if (!g_console_input_config.collect_number) {
        xTimerStop(g_collect_timer_handele, 0);
        xTimerDelete(g_collect_timer_handele, 0);
        g_collect_timer_handele = NULL;
        strcpy(g_console_input_config.collect_taget, "unknown");
        return;
    }
}

static int wifi_cmd_radar(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &radar_args) != ESP_OK) {
        arg_print_errors(stderr, radar_args.end, argv[0]);
        return ESP_FAIL;
    }

    if (radar_args.train_start->count) {
        if (!radar_args.train_add->count) {
            esp_radar_train_remove();
        }

        esp_radar_train_start();
        g_console_input_config.train_start = true;
    }

    if (radar_args.train_stop->count) {
        esp_radar_train_stop(&g_console_input_config.predict_someone_threshold,
                             &g_console_input_config.predict_move_threshold);
        g_console_input_config.train_start = false;

        printf("RADAR_DADA,0,0,0,%.6f,0,0,%.6f,0\n", 
                g_console_input_config.predict_someone_threshold,
                g_console_input_config.predict_move_threshold);
    }

    if (radar_args.predict_move_threshold->count) {
        g_console_input_config.predict_move_threshold = atof(radar_args.predict_move_threshold->sval[0]);
    }

    if (radar_args.predict_move_sensitivity->count) {
        g_console_input_config.predict_move_sensitivity = atof(radar_args.predict_move_sensitivity->sval[0]);
        ESP_LOGI(TAG, "predict_move_sensitivity: %f", g_console_input_config.predict_move_sensitivity);
    }

    if (radar_args.predict_someone_threshold->count) {
        g_console_input_config.predict_someone_threshold = atof(radar_args.predict_someone_threshold->sval[0]);
    }

    if (radar_args.predict_someone_sensitivity->count) {
        g_console_input_config.predict_someone_sensitivity = atof(radar_args.predict_someone_sensitivity->sval[0]);
        ESP_LOGI(TAG, "predict_someone_sensitivity: %f", g_console_input_config.predict_someone_sensitivity);
    }

    if (radar_args.predict_buff_size->count) {
        g_console_input_config.predict_buff_size = radar_args.predict_buff_size->ival[0];
    }

    if (radar_args.predict_outliers_number->count) {
        g_console_input_config.predict_outliers_number = radar_args.predict_outliers_number->ival[0];
    }

    if (radar_args.collect_taget->count && radar_args.collect_number->count && radar_args.collect_duration->count) {
        g_console_input_config.collect_number = radar_args.collect_number->ival[0];
        strcpy(g_console_input_config.collect_taget, radar_args.collect_taget->sval[0]);

        if (g_collect_timer_handele) {
            xTimerStop(g_collect_timer_handele, portMAX_DELAY);
            xTimerDelete(g_collect_timer_handele, portMAX_DELAY);
        }

        g_collect_timer_handele = xTimerCreate("collect", pdMS_TO_TICKS(radar_args.collect_duration->ival[0]),
                                               true, NULL, collect_timercb);
        xTimerStart(g_collect_timer_handele, portMAX_DELAY);
    }

    if (radar_args.csi_output_format->count) {
        strcpy(g_console_input_config.csi_output_format, radar_args.csi_output_format->sval[0]);
    }

    if (radar_args.csi_output_type->count) {
        wifi_radar_config_t radar_config = {0};
        esp_radar_get_config(&radar_config);

        if (!strcasecmp(radar_args.csi_output_type->sval[0], "NULL")) {
            radar_config.wifi_csi_filtered_cb = NULL;
        } else {
            void wifi_csi_raw_cb(const wifi_csi_filtered_info_t *info, void *ctx);
            radar_config.wifi_csi_filtered_cb = wifi_csi_raw_cb;
            strcpy(g_console_input_config.csi_output_type, radar_args.csi_output_type->sval[0]);
        }

        esp_radar_set_config(&radar_config);
    }

    if (radar_args.csi_start->count) {
        esp_radar_start();
    }

    if (radar_args.csi_stop->count) {
        esp_radar_stop();
    }

    if (radar_args.csi_scale_shift->count) {
        wifi_radar_config_t radar_config = {0};
        esp_radar_get_config(&radar_config);
        radar_config.csi_config.shift = radar_args.csi_scale_shift->ival[0];
        esp_radar_set_config(&radar_config);

        ESP_LOGI(TAG, "manually left shift %d bits of the scale of the CSI data", radar_config.csi_config.shift);
    }

    if (radar_args.channel_filter->count) {
        wifi_radar_config_t radar_config = {0};
        esp_radar_get_config(&radar_config);
        radar_config.csi_config.channel_filter_en = radar_args.channel_filter->ival[0];
        esp_radar_set_config(&radar_config);

        ESP_LOGI(TAG, "enable(%d) to turn on channel filter to smooth adjacent sub-carrier", radar_config.csi_config.channel_filter_en);
    }

    if (radar_args.send_data_interval->count) {
        g_send_data_interval = radar_args.send_data_interval->ival[0];
    }

    return ESP_OK;
}

void cmd_register_radar(void)
{
    radar_args.train_start = arg_lit0(NULL, "train_start", "Start calibrating the 'Radar' algorithm");
    radar_args.train_stop  = arg_lit0(NULL, "train_stop", "Stop calibrating the 'Radar' algorithm");
    radar_args.train_add   = arg_lit0(NULL, "train_add", "Calibrate on the basis of saving the calibration results");

    radar_args.predict_someone_threshold = arg_str0(NULL, "predict_someone_threshold", "<0 ~ 1.0>", "Configure the threshold for someone");
    radar_args.predict_someone_sensitivity  = arg_str0(NULL, "predict_someone_sensitivity", "<0 ~ 1.0>", "Configure the sensitivity for someone");
    radar_args.predict_move_threshold    = arg_str0(NULL, "predict_move_threshold", "<0 ~ 1.0>", "Configure the threshold for move");
    radar_args.predict_move_sensitivity  = arg_str0(NULL, "predict_move_sensitivity", "<0 ~ 1.0>", "Configure the sensitivity for move");
    radar_args.predict_buff_size         = arg_int0(NULL, "predict_buff_size", "1 ~ 100", "Buffer size for filtering outliers");
    radar_args.predict_outliers_number   = arg_int0(NULL, "predict_outliers_number", "<1 ~ 100>", "The number of items in the buffer queue greater than the threshold");

    radar_args.collect_taget    = arg_str0(NULL, "collect_tagets", "<0 ~ 20>", "Type of CSI data collected");
    radar_args.collect_number   = arg_int0(NULL, "collect_number", "sequence", "Number of times CSI data was collected");
    radar_args.collect_duration = arg_int0(NULL, "collect_duration", "duration", "Time taken to acquire one CSI data");

    radar_args.csi_start         = arg_lit0(NULL, "csi_start", "Start collecting CSI data from Wi-Fi");
    radar_args.csi_stop          = arg_lit0(NULL, "csi_stop", "Stop CSI data collection from Wi-Fi");
    radar_args.csi_output_type   = arg_str0(NULL, "csi_output_type", "<NULL, LLFT, HT-LFT, STBC-HT-LTF>", "Type of CSI data");
    radar_args.csi_output_format = arg_str0(NULL, "csi_output_format", "<decimal, base64>", "Format of CSI data");
    radar_args.csi_scale_shift   = arg_int0(NULL, "scale_shift", "<0~15>", "manually left shift bits of the scale of the CSI data");
    radar_args.channel_filter    = arg_int0(NULL, "channel_filter", "<0 or 1>", "enable to turn on channel filter to smooth adjacent sub-carrier");
    
    radar_args.send_data_interval = arg_int0(NULL, "send_data_interval", "<interval_ms>", "The interval between sending null data or ping packets to the router");

    radar_args.end                = arg_end(8);

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
    char *buffer = malloc(8 * 1024);
    static uint32_t count = 0;

    while (xQueueReceive(g_csi_info_queue, &info, portMAX_DELAY)) {
        size_t len = 0;
        wifi_pkt_rx_ctrl_t *rx_ctrl = &info->rx_ctrl;

        if (!count) {
            ESP_LOGI(TAG, "================ CSI RECV ================");
            len += sprintf(buffer + len, "type,sequence,timestamp,taget_seq,taget,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,agc_gain,fft_gain,len,first_word,data\n");
        }

        if (!strcasecmp(g_console_input_config.csi_output_type, "LLFT")) {
            info->valid_len = info->valid_llft_len;
        } else if (!strcasecmp(g_console_input_config.csi_output_type, "HT-LFT")) {
            info->valid_len = info->valid_llft_len + info->valid_ht_lft_len;
        } else if (!strcasecmp(g_console_input_config.csi_output_type, "STBC-HT-LTF")) {
            info->valid_len = info->valid_llft_len + info->valid_ht_lft_len + info->valid_stbc_ht_lft_len;
        }

        len += sprintf(buffer + len, "CSI_DATA,%d,%u,%u,%s," MACSTR ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%d,%d,%d,%d,%d,%d,%d,",
                       count++, esp_log_timestamp(), g_console_input_config.collect_number, g_console_input_config.collect_taget,
                       MAC2STR(info->mac), rx_ctrl->rssi, rx_ctrl->rate, rx_ctrl->sig_mode,
                       rx_ctrl->mcs, rx_ctrl->cwb, rx_ctrl->smoothing, rx_ctrl->not_sounding,
                       rx_ctrl->aggregation, rx_ctrl->stbc, rx_ctrl->fec_coding, rx_ctrl->sgi,
                       rx_ctrl->noise_floor, rx_ctrl->ampdu_cnt, rx_ctrl->channel, rx_ctrl->secondary_channel,
                       rx_ctrl->timestamp, rx_ctrl->ant, rx_ctrl->sig_len, rx_ctrl->rx_state, info->agc_gain, info->fft_gain, info->valid_len, 0);

        if (!strcasecmp(g_console_input_config.csi_output_format, "base64")) {
            size_t size = 0;
            mbedtls_base64_encode((uint8_t *)buffer + len, sizeof(buffer) - len, &size, (uint8_t *)info->valid_data, info->valid_len);
            len += size;
            len += sprintf(buffer + len, "\n");
        } else {
            len += sprintf(buffer + len, "\"[%d", info->valid_data[0]);

            for (int i = 1; i < info->valid_len; i++) {
                len += sprintf(buffer + len, ",%d", info->valid_data[i]);
            }

            len += sprintf(buffer + len, "]\"\n");
        }

        printf("%s", buffer);
        free(info);
    }

    free(buffer);
    vTaskDelete(NULL);
}

void wifi_csi_raw_cb(const wifi_csi_filtered_info_t *info, void *ctx)
{
    wifi_csi_filtered_info_t *q_data = malloc(sizeof(wifi_csi_filtered_info_t) + info->valid_len);
    *q_data = *info;
    memcpy(q_data->valid_data, info->valid_data, info->valid_len);

    if (!g_csi_info_queue || xQueueSend(g_csi_info_queue, &q_data, 0) == pdFALSE) {
        ESP_LOGW(TAG, "g_csi_info_queue full");
        free(q_data);
    }
}

static void wifi_radar_cb(const wifi_radar_info_t *info, void *ctx)
{
    static float *s_buff_wander  = NULL;
    static float *s_buff_jitter  = NULL;
    static uint32_t s_buff_count = 0;
    uint32_t buff_max_size       = g_console_input_config.predict_buff_size;
    uint32_t buff_outliers_num   = g_console_input_config.predict_outliers_number;
    uint32_t someone_count       = 0;
    uint32_t move_count          = 0;
    bool room_status             = false;
    bool human_status            = false;

    if (!s_buff_wander) {
        s_buff_wander = calloc(RADAR_BUFF_MAX_LEN, sizeof(float));
    }

    if (!s_buff_jitter) {
        s_buff_jitter = calloc(RADAR_BUFF_MAX_LEN, sizeof(float));
    }

    s_buff_wander[s_buff_count % RADAR_BUFF_MAX_LEN] = info->waveform_wander;
    s_buff_jitter[s_buff_count % RADAR_BUFF_MAX_LEN] = info->waveform_jitter;
    s_buff_count++;

    if (s_buff_count < buff_max_size) {
        return;
    }

    extern float trimmean(const float *array, size_t len, float percent);
    extern float median(const float *a, size_t len);

    float wander_average = trimmean(s_buff_wander, RADAR_BUFF_MAX_LEN, 0.5);
    float jitter_midean = median(s_buff_jitter, RADAR_BUFF_MAX_LEN);

    for (int i = 0; i < buff_max_size; i++) {
        uint32_t index = (s_buff_count - 1 - i) % RADAR_BUFF_MAX_LEN;

        if ((wander_average * g_console_input_config.predict_someone_sensitivity > g_console_input_config.predict_someone_threshold)) {
            someone_count++;
        }

        if (s_buff_jitter[index] * g_console_input_config.predict_move_sensitivity > g_console_input_config.predict_move_threshold
            || (s_buff_jitter[index] * g_console_input_config.predict_move_sensitivity > jitter_midean && s_buff_jitter[index] > 0.0002)) {
            move_count++;
        }
    }

    if (someone_count >= 1) {
        room_status = true;
    }

    if (move_count >= buff_outliers_num) {
        human_status = true;
    }

    static uint32_t s_count = 0;

    if (!s_count) {
        ESP_LOGI(TAG, "================ RADAR RECV ================");
        ESP_LOGI(TAG, "type,sequence,timestamp,waveform_wander,someone_threshold,someone_status,waveform_jitter,move_threshold,move_status");
    }

    char timestamp_str[32] = {0};
    sprintf(timestamp_str, "%u", esp_log_timestamp());

    if (ctx) {
        strncpy(timestamp_str, (char *)ctx, 31);
    }

    static uint32_t s_last_move_time    = 0;
    static uint32_t s_last_someone_time = 0;

    if (g_console_input_config.train_start) {
        s_last_move_time    = esp_log_timestamp();
        s_last_someone_time = esp_log_timestamp();

        static bool led_status = false;

        if (led_status) {
            ws2812_led_set_rgb(0, 0, 0);
        } else {
            ws2812_led_set_rgb(255, 255, 0);
        }

        led_status = !led_status;

        return;
    }

    printf("RADAR_DADA,%d,%s,%.6f,%.6f,%.6f,%d,%.6f,%.6f,%.6f,%d\n",
           s_count++, timestamp_str,
           info->waveform_wander, wander_average, g_console_input_config.predict_someone_threshold / g_console_input_config.predict_someone_sensitivity, room_status,
           info->waveform_jitter, jitter_midean, jitter_midean / g_console_input_config.predict_move_sensitivity, human_status);

    if (room_status) {
        if (human_status) {
            ws2812_led_set_rgb(0, 255, 0);
            ESP_LOGI(TAG, "Someone moved");
            s_last_move_time = esp_log_timestamp();
        } else if (esp_log_timestamp() - s_last_move_time > 3 * 1000) {
            ws2812_led_set_rgb(255, 255, 255);
            ESP_LOGI(TAG, "Someone");
        }

        s_last_someone_time = esp_log_timestamp();
    } else if (esp_log_timestamp() - s_last_someone_time > 3 * 1000) {
        if (human_status) {
            s_last_move_time = esp_log_timestamp();
            ws2812_led_set_rgb(255, 0, 0);
        } else if (esp_log_timestamp() - s_last_move_time > 3 * 1000) {
            ws2812_led_set_rgb(0, 0, 0);
        }
    }
}

static void trigger_router_send_data_task(void *arg)
{
    wifi_radar_config_t radar_config = {0};
    wifi_ap_record_t ap_info         = {0};
    uint8_t sta_mac[6]               = {0};

    esp_radar_get_config(&radar_config);
    esp_wifi_sta_get_ap_info(&ap_info);
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, sta_mac));

    radar_config.csi_recv_interval = g_send_data_interval;
    memcpy(radar_config.filter_dmac, sta_mac, sizeof(radar_config.filter_dmac));

#if WIFI_CSI_SEND_NULL_DATA_ENABLE
    ESP_LOGI(TAG, "Send null data to router");

    memset(radar_config.filter_mac, 0, sizeof(radar_config.filter_mac));
    esp_radar_set_config(&radar_config);

typedef struct {
    uint8_t frame_control[2];
    uint16_t duration;
    uint8_t destination_address[6];
    uint8_t source_address[6];
    uint8_t broadcast_address[6];
    uint16_t sequence_control;
} __attribute__((packed)) wifi_null_data_t;

    wifi_null_data_t null_data = {
        .frame_control       = {0x48, 0x01},
        .duration            = 0x0000,
        .sequence_control    = 0x0000,
    };

    memcpy(null_data.destination_address, ap_info.bssid, 6);
    memcpy(null_data.broadcast_address, ap_info.bssid, 6);
    memcpy(null_data.source_address, sta_mac, 6);

    ESP_LOGW(TAG, "null_data, destination_address: "MACSTR", source_address: "MACSTR", broadcast_address: " MACSTR,
             MAC2STR(null_data.destination_address), MAC2STR(null_data.source_address), MAC2STR(null_data.broadcast_address));

    ESP_ERROR_CHECK(esp_wifi_config_80211_tx_rate(WIFI_IF_STA, WIFI_PHY_RATE_6M));

    for (int i = 0; g_wifi_connect_status; i++) {
        esp_err_t ret = esp_wifi_80211_tx(WIFI_IF_STA, &null_data, sizeof(wifi_null_data_t), true);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "esp_wifi_80211_tx, %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        vTaskDelay(pdMS_TO_TICKS(g_send_data_interval));
    }

#else
    ESP_LOGI(TAG, "Send ping data to router");

    memcpy(radar_config.filter_mac, ap_info.bssid, sizeof(radar_config.filter_mac));
    esp_radar_set_config(&radar_config);

    static esp_ping_handle_t ping_handle = NULL;
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.count       = 0;
    config.data_size   = 1;
    config.interval_ms = g_send_data_interval;

    /**
     * @brief Get the Router IP information from the esp-netif
     */
    esp_netif_ip_info_t local_ip;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &local_ip);
    ESP_LOGI(TAG, "Ping: got ip:" IPSTR ", gw: " IPSTR, IP2STR(&local_ip.ip), IP2STR(&local_ip.gw));
    config.target_addr.u_addr.ip4.addr = ip4_addr_get_u32(&local_ip.gw);
    config.target_addr.type = ESP_IPADDR_TYPE_V4;

    esp_ping_callbacks_t cbs = { 0 };
    esp_ping_new_session(&config, &cbs, &ping_handle);
    esp_ping_start(ping_handle);
#endif

    vTaskDelete(NULL);
}

/* Event handler for catching system events */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        g_wifi_connect_status = true;

        xTaskCreate(trigger_router_send_data_task, "trigger_router_send_data", 4 * 1024, NULL, 5, NULL);

#ifdef RECV_ESPNOW_CSI
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
#endif
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        g_wifi_connect_status = false;
        ESP_LOGW(TAG, "Wi-Fi disconnected");
    }
}

void app_main(void)
{
    /**
     * @brief Install ws2812 driver, Used to display the status of the device
     */
    ws2812_led_init();
    ws2812_led_clear();

    /**
     * @brief Turn on the radar module printing information
     */
    esp_log_level_set("esp_radar", ESP_LOG_INFO);

    /**
     * @brief Register serial command
     */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    repl_config.prompt = "csi>";
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
    /**< Fix serial port garbled code due to high baud rate */
    uart_ll_set_sclk(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM), UART_SCLK_APB);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    uart_ll_set_baudrate(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM), CONFIG_ESP_CONSOLE_UART_BAUDRATE, APB_CLK_FREQ);
#else
    uart_ll_set_baudrate(UART_LL_GET_HW(CONFIG_ESP_CONSOLE_UART_NUM), CONFIG_ESP_CONSOLE_UART_BAUDRATE);
#endif
#endif
    wifi_init();

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_event_handler, NULL));

    cmd_register_ping();
    cmd_register_system();
    cmd_register_wifi_config();
    cmd_register_wifi_scan();
    cmd_register_radar();
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    /**
     * @brief Initialize Wi-Fi Radar
     */
    esp_radar_init();

    /**
     * @brief Set the Wi-Fi radar configuration
     */
    wifi_radar_config_t radar_config = WIFI_RADAR_CONFIG_DEFAULT();
    radar_config.wifi_radar_cb     = wifi_radar_cb;
    radar_config.csi_recv_interval = g_send_data_interval;

#if WIFI_CSI_SEND_NULL_DATA_ENABLE
    radar_config.csi_config.dump_ack_en       = true;
#endif
    memcpy(radar_config.filter_mac, "\x1a\x00\x00\x00\x00\x00", 6);
    esp_radar_set_config(&radar_config);

    /**
     * @brief Start Wi-Fi radar
     */
    esp_radar_start();

    /**
     * @brief Initialize CSI serial port printing task, Use tasks to avoid blocking wifi_csi_raw_cb
     */
    g_csi_info_queue = xQueueCreate(64, sizeof(void *));
    xTaskCreate(csi_data_print_task, "csi_data_print", 4 * 1024, NULL, 0, NULL);
}
