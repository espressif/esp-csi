#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/timers.h"

#include <esp_log.h>
#include <nvs_flash.h>

#include "ping/ping_sock.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_utils.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_common_events.h>
#include <esp_rmaker_standard_types.h>

#include <app_wifi.h>
#include <app_reset.h>
#include <app_insights.h>
#include <ws2812_led.h>
#include <iot_button.h>

#include "esp_radar.h"

#ifdef CONFIG_IDF_TARGET_ESP32C3
#define BUTTON_GPIO                                GPIO_NUM_8
#else
#define BUTTON_GPIO                                GPIO_NUM_0
#endif

#define BUTTON_WIFI_RESET_TIMEOUT                  3
#define BUTTON_FACTORY_RESET_TIMEOUT               5
#define AUTO_CALIBRATE_RAPORT_INTERVAL             5

#define RADAR_PING_DATA_INTERVAL                   10
#define RADAR_DETECT_BUFF_MAX_SIZE                 16
#define RADAR_PARAM_SOMEONE_STATUS                 "someone_status"
#define RADAR_PARAM_SOMEONE_TIMEOUT                "someone_timeout"
#define RADAR_PARAM_MOVE_STATUS                    "move_status"
#define RADAR_PARAM_MOVE_COUNT                     "move_count"
#define RADAR_PARAM_MOVE_THRESHOLD                 "move_threshold"
#define RADAR_PARAM_FILTER_WINDOW                  "filter_window"
#define RADAR_PARAM_FILTER_COUNT                   "filter_count"
#define RADAR_PARAM_THRESHOLD_CALIBRATE            "threshold_calibrate"
#define RADAR_PARAM_THRESHOLD_CALIBRATE_TIMEOUT    "threshold_calibrate_timeout"

typedef struct  {
    bool threshold_calibrate;               /**< Self-calibration acquisition, the most suitable threshold, calibration is to ensure that no one is in the room */
    uint8_t threshold_calibrate_timeout;    /**< Calibration timeout, the unit is second */
    float someone_timeout;                  /**< The unit is second, how long no one moves in the room is marked as no one */
    float move_threshold;                   /**< Use to determine whether someone is moving */
    uint8_t filter_window;                  /**< window without filtering outliers */
    uint8_t filter_count;                   /**< Detect multiple times within the outlier window greater than move_threshold to mark as someone moving */
} radar_detect_config_t;

static const char *TAG                            = "app_main";
static float g_someone_threshold                  = 0.002;
static nvs_handle_t g_nvs_handle                  = 0;
static int32_t g_auto_calibrate_timerleft         = 0;
static TimerHandle_t g_auto_calibrate_timerhandle = NULL;
static esp_rmaker_device_t *radar_device          = NULL;
static radar_detect_config_t g_detect_config      = {
    .someone_timeout = 3 * 60,
    .move_threshold  = 0.002,
    .filter_window   = 5,
    .filter_count    = 2,
    .threshold_calibrate_timeout = 60,
};

static esp_err_t ping_router_start(uint32_t interval_ms)
{
    static esp_ping_handle_t ping_handle = NULL;
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.count       = 0;
    config.timeout_ms  = 1000;
    config.data_size   = 1;
    config.interval_ms = interval_ms;

    /**
     * @brief Get the Router IP information from the esp-netif
     */
    esp_netif_ip_info_t local_ip;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &local_ip);
    ESP_LOGI(TAG, "got ip:" IPSTR ", gw: " IPSTR, IP2STR(&local_ip.ip), IP2STR(&local_ip.gw));
    config.target_addr.u_addr.ip4.addr = ip4_addr_get_u32(&local_ip.gw);
    config.target_addr.type = ESP_IPADDR_TYPE_V4;

    esp_ping_callbacks_t cbs = { 0 };
    esp_ping_new_session(&config, &cbs, &ping_handle);
    esp_ping_start(ping_handle);

    return ESP_OK;
}

static void radar_cb(const wifi_radar_info_t *info, void *ctx)
{
    bool someone_status = false;
    bool move_status    = false;

    /**
     * @brief Filtering outliers is more accurate using detection,
     *        but it takes more time, lower sensitive to the environment
     *
     * @note  Current algorithm limitations:
     *        1. Calibration is required to detect whether there are people in the room;
     *        2. No calibration is required to detect movement, but the sensitivity is higher after calibration
     */
    static uint32_t s_buff_count = 0;
    static float s_buff_jitter[RADAR_DETECT_BUFF_MAX_SIZE] = {0};
    uint32_t buff_max_size     = g_detect_config.filter_window;
    uint32_t buff_outliers_num = g_detect_config.filter_count;

    s_buff_jitter[s_buff_count % buff_max_size] = info->waveform_jitter;

    if (++s_buff_count < buff_max_size) {
        return;
    }

    for (int i = 0, move_count = 0; i < buff_max_size; i++) {
        if (s_buff_jitter[i] > g_detect_config.move_threshold) {
            move_count++;
        }

        if (move_count >= buff_outliers_num) {
            move_status = true;
        }
    }

    someone_status = (info->waveform_jitter > g_someone_threshold)? true: false;
    static uint32_t s_count = 0;

    if (!s_count++) {
        ESP_LOGI(TAG, "================ RADAR RECV ================");
        ESP_LOGI(TAG, "type,sequence,timestamp,waveform_wander,someone_threshold,someone_status,waveform_jitter,move_threshold,move_status\n");
    }

    ESP_LOGI("RADAR_DADA", "RADAR_DADA,%d,%u,%.6f,%.6f,%d,%.6f,%.6f,%d",
             s_count, esp_log_timestamp(),
             info->waveform_wander, g_someone_threshold, someone_status,
             info->waveform_jitter, g_detect_config.move_threshold, move_status);

    /* Calibrate the environment, LED will flash yellow */
    if (g_detect_config.threshold_calibrate) {
        static bool led_status = false;

        if (led_status) {
            ws2812_led_set_rgb(0, 0, 0);
        } else {
            ws2812_led_set_rgb(255, 255, 0);
        }

        led_status = !led_status;
        return;
    }

    /**
     * @brief Use LED and LOG to display the detection results
     *        1. Someone moves in the room, LED will flash green
     *        2. No one moves in the room, LED will flash white
     *        3. No one in the room, LED will trun off
     */
    static uint32_t s_last_move_time = 0;
    static bool s_last_move_status   = false;

    if (esp_log_timestamp() - s_last_move_time < g_detect_config.someone_timeout * 1000) {
        someone_status = true;
    } else {
        someone_status = false;
    }

    if (move_status) {
        if (move_status != s_last_move_status) {
            ws2812_led_set_rgb(0, 255, 0);
            ESP_LOGI(TAG, "someone moves in the room");
        }

        s_last_move_time = esp_log_timestamp();
    } else if (move_status != s_last_move_status) {
        ESP_LOGI(TAG, "No one moves in the room");
    } else if (esp_log_timestamp() - s_last_move_time > 3 * 1000) {
        if (someone_status) {
            ws2812_led_set_rgb(255, 255, 255);
        } else {
            ws2812_led_set_rgb(0, 0, 0);
        }
    }

    s_last_move_status = move_status;

    /**
     * @brief Report the room status to the cloud every 10 seconds or when the room status changes
     */
    if (!esp_rmaker_time_check()) {
        return;
    }

    static bool s_report_someone_status     = false;
    static bool s_report_move_status        = false;
    static uint32_t s_report_move_count     = 0;
    static uint32_t s_report_move_timestamp = 0;
    s_report_move_count = move_status ? s_report_move_count + 1: s_report_move_count;

    if (s_report_someone_status != someone_status && !someone_status) {
        esp_rmaker_param_t *someone_status_param = esp_rmaker_device_get_param_by_name(radar_device, RADAR_PARAM_SOMEONE_STATUS);
        esp_rmaker_param_update_and_report(someone_status_param, esp_rmaker_bool(someone_status));
        s_report_someone_status = someone_status;
    }

    if (move_status != s_report_move_status
            /* Reduce the number of times to report data and reduce server pressure */
            || (move_status && esp_log_timestamp() - s_report_move_timestamp > 10 * 1000)) {
        esp_rmaker_param_t *someone_status_param = esp_rmaker_device_get_param_by_name(radar_device, RADAR_PARAM_SOMEONE_STATUS);
        esp_rmaker_param_update(someone_status_param, esp_rmaker_bool(someone_status));
        esp_rmaker_param_t *move_status_param = esp_rmaker_device_get_param_by_name(radar_device, RADAR_PARAM_MOVE_STATUS);
        esp_rmaker_param_update(move_status_param, esp_rmaker_bool(move_status | (s_report_move_count > 0)));
        esp_rmaker_param_t *move_count_param = esp_rmaker_device_get_param_by_name(radar_device, RADAR_PARAM_MOVE_COUNT);
        esp_rmaker_param_update_and_report(move_count_param, esp_rmaker_int(s_report_move_count));

        if (!s_report_move_status || !s_report_move_count) {
            s_report_move_status = move_status;
        }

        s_report_move_count     = 0;
        s_report_move_timestamp = esp_log_timestamp();
    }
}

static esp_err_t radar_start()
{
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    wifi_radar_config_t radar_config = {
        .wifi_radar_cb = radar_cb,
    };

    wifi_ap_record_t ap_info = {0x0};

    esp_wifi_sta_get_ap_info(&ap_info);
    memcpy(radar_config.filter_mac, ap_info.bssid, sizeof(ap_info.bssid));

    esp_radar_init();
    esp_radar_set_config(&radar_config);
    esp_radar_start();

    return ESP_OK;
}

static void auto_calibrate_timercb(void *timer)
{
    g_auto_calibrate_timerleft -= AUTO_CALIBRATE_RAPORT_INTERVAL;

    if (g_auto_calibrate_timerleft < AUTO_CALIBRATE_RAPORT_INTERVAL) {
        g_auto_calibrate_timerleft = g_detect_config.threshold_calibrate_timeout;
        g_detect_config.threshold_calibrate = false;
        xTimerStop(g_auto_calibrate_timerhandle, portMAX_DELAY);

        esp_radar_train_stop(&g_someone_threshold, &g_detect_config.move_threshold);
        esp_rmaker_param_t *move_threshold_param = esp_rmaker_device_get_param_by_name(radar_device, RADAR_PARAM_MOVE_THRESHOLD);
        esp_rmaker_param_update(move_threshold_param, esp_rmaker_float(g_detect_config.move_threshold));

        nvs_set_blob(g_nvs_handle, "detect_config", &g_detect_config, sizeof(radar_detect_config_t));
        nvs_commit(g_nvs_handle);
    }

    esp_rmaker_param_t *param_threshold_calibrate = esp_rmaker_device_get_param_by_name(radar_device, RADAR_PARAM_THRESHOLD_CALIBRATE);
    esp_rmaker_param_update(param_threshold_calibrate, esp_rmaker_bool(g_detect_config.threshold_calibrate));
    esp_rmaker_param_t *param_threshold_calibrate_timeout = esp_rmaker_device_get_param_by_name(radar_device, RADAR_PARAM_THRESHOLD_CALIBRATE_TIMEOUT);
    esp_rmaker_param_update_and_report(param_threshold_calibrate_timeout, esp_rmaker_int(g_auto_calibrate_timerleft));
}

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);

    if (strcmp(param_name, RADAR_PARAM_SOMEONE_TIMEOUT) == 0) {
        g_detect_config.someone_timeout = val.val.i;
    } else if (strcmp(param_name, RADAR_PARAM_MOVE_THRESHOLD) == 0) {
        g_detect_config.move_threshold = val.val.f;
    } else if (strcmp(param_name, RADAR_PARAM_FILTER_WINDOW) == 0) {
        g_detect_config.filter_window = val.val.i;
    } else if (strcmp(param_name, RADAR_PARAM_FILTER_COUNT) == 0) {
        g_detect_config.filter_count = val.val.i;
    } else if (strcmp(param_name, RADAR_PARAM_THRESHOLD_CALIBRATE) == 0) {
        g_detect_config.threshold_calibrate = val.val.b;

        if (!g_auto_calibrate_timerhandle) {
            g_auto_calibrate_timerhandle = xTimerCreate("auto_calibrate", pdMS_TO_TICKS(AUTO_CALIBRATE_RAPORT_INTERVAL * 1000),
                                           true, NULL, auto_calibrate_timercb);
        }

        if (g_detect_config.threshold_calibrate) {
            g_auto_calibrate_timerleft = g_detect_config.threshold_calibrate_timeout;
            xTimerStart(g_auto_calibrate_timerhandle, portMAX_DELAY);
            esp_radar_train_start();
        } else {
            g_auto_calibrate_timerleft = 0;
            auto_calibrate_timercb(NULL);
        }
    } else if (strcmp(param_name, RADAR_PARAM_THRESHOLD_CALIBRATE_TIMEOUT) == 0) {
        g_detect_config.threshold_calibrate_timeout = val.val.i;
    } else {
        ESP_LOGE(TAG, "Unhandled parameter '%s'", param_name);
        /* Silently ignoring invalid params */
        return ESP_OK;
    }

    esp_rmaker_param_update_and_report(param, val);
    nvs_set_blob(g_nvs_handle, "detect_config", &g_detect_config, sizeof(radar_detect_config_t));
    nvs_commit(g_nvs_handle);
    return ESP_OK;
}

void app_main()
{
    /* Initialize Application specific hardware drivers and set initial state */
    ws2812_led_init();
    ws2812_led_clear();
    button_handle_t btn_handle = iot_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LOW);
    app_reset_button_register(btn_handle, BUTTON_WIFI_RESET_TIMEOUT, BUTTON_FACTORY_RESET_TIMEOUT);

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    nvs_open("wifi_radar", NVS_READWRITE, &g_nvs_handle);
    size_t detect_config_size = sizeof(radar_detect_config_t);
    nvs_get_blob(g_nvs_handle, "detect_config", &g_detect_config, &detect_config_size);

    /* Reduced output log */
    esp_log_level_set("esp_radar", ESP_LOG_WARN);
    esp_log_level_set("RADAR_DADA", ESP_LOG_WARN);

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init() */
    app_wifi_init();

    /* Initialize the ESP RainMaker Agent */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = true,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP Wi-Fi CSI Device", "Wi-Fi Radar");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(pdMS_TO_TICKS(5000));
        abort();
    }

    /* Create a device and add the relevant parameters to it */
    radar_device = esp_rmaker_device_create("Sensor Light", ESP_RMAKER_DEVICE_LIGHTBULB, NULL);
    ESP_ERROR_CHECK(esp_rmaker_device_add_cb(radar_device, write_cb, NULL));

    typedef struct {
        const char *param_name;
        uint8_t properties;
        const char *ui_type;
        esp_rmaker_param_val_t val;
        esp_rmaker_param_val_t min;
        esp_rmaker_param_val_t max;
        esp_rmaker_param_val_t step;
    } radar_param_t;
    esp_rmaker_param_val_t invalid_val = {.type = RMAKER_VAL_TYPE_INVALID};
    radar_param_t radar_param_list[] = {
        {RADAR_PARAM_THRESHOLD_CALIBRATE, PROP_FLAG_READ | PROP_FLAG_WRITE, ESP_RMAKER_UI_TOGGLE, esp_rmaker_bool(g_detect_config.threshold_calibrate),invalid_val,invalid_val,invalid_val},
        {RADAR_PARAM_THRESHOLD_CALIBRATE_TIMEOUT, PROP_FLAG_READ | PROP_FLAG_WRITE, ESP_RMAKER_UI_TEXT, esp_rmaker_int(g_detect_config.threshold_calibrate_timeout), esp_rmaker_int(10), esp_rmaker_int(3600), esp_rmaker_int(10)},
        {RADAR_PARAM_MOVE_THRESHOLD, PROP_FLAG_READ | PROP_FLAG_WRITE, ESP_RMAKER_UI_TEXT, esp_rmaker_float(g_detect_config.move_threshold), esp_rmaker_float(0.01), esp_rmaker_float(0.1), esp_rmaker_float(0.01)},
        {RADAR_PARAM_MOVE_STATUS, PROP_FLAG_READ, ESP_RMAKER_UI_TEXT, esp_rmaker_bool(false),invalid_val,invalid_val,invalid_val},
        {RADAR_PARAM_MOVE_COUNT, PROP_FLAG_READ | PROP_FLAG_TIME_SERIES, ESP_RMAKER_UI_TEXT, esp_rmaker_int(0), esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1)},
        {RADAR_PARAM_SOMEONE_STATUS, PROP_FLAG_READ, ESP_RMAKER_UI_TEXT, esp_rmaker_bool(false),invalid_val,invalid_val,invalid_val},
        {RADAR_PARAM_SOMEONE_TIMEOUT, PROP_FLAG_READ | PROP_FLAG_WRITE, ESP_RMAKER_UI_TEXT, esp_rmaker_int(g_detect_config.someone_timeout), esp_rmaker_int(10), esp_rmaker_int(3600), esp_rmaker_int(10)},
        {RADAR_PARAM_FILTER_WINDOW, PROP_FLAG_READ | PROP_FLAG_WRITE, ESP_RMAKER_UI_TEXT, esp_rmaker_int(g_detect_config.filter_window), esp_rmaker_int(1), esp_rmaker_int(16), esp_rmaker_int(1)},
        {RADAR_PARAM_FILTER_COUNT, PROP_FLAG_READ | PROP_FLAG_WRITE, ESP_RMAKER_UI_TEXT, esp_rmaker_int(g_detect_config.filter_count), esp_rmaker_int(1), esp_rmaker_int(16), esp_rmaker_int(1)},
    };

    for (int i = 0; i < sizeof(radar_param_list) / sizeof(radar_param_list[0]); ++i) {
        char radar_param_type[64] = "esp.param.";
        strcat(radar_param_type, radar_param_list[i].param_name);
        esp_rmaker_param_t *param = esp_rmaker_param_create(radar_param_list[i].param_name, radar_param_type,
                                    radar_param_list[i].val, radar_param_list[i].properties);

        if(radar_param_list[i].ui_type) {
            esp_rmaker_param_add_ui_type(param, radar_param_list[i].ui_type);
        }

        if (radar_param_list[i].min.type != RMAKER_VAL_TYPE_INVALID) {
            esp_rmaker_param_add_bounds(param, radar_param_list[i].min, radar_param_list[i].max, radar_param_list[i].step);
        }

        ESP_ERROR_CHECK(esp_rmaker_device_add_param(radar_device, param));
    }

    esp_rmaker_param_t *move_count_param = esp_rmaker_device_get_param_by_name(radar_device, RADAR_PARAM_MOVE_COUNT);
    esp_rmaker_device_assign_primary_param(radar_device, move_count_param);
    ESP_ERROR_CHECK(esp_rmaker_node_add_device(node, radar_device));

    /* Enable OTA */
    ESP_ERROR_CHECK(esp_rmaker_ota_enable_default());

    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();

    /* Enable scheduling. */
    esp_rmaker_schedule_enable();

    /* Enable Scenes */
    esp_rmaker_scenes_enable();

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    app_insights_enable();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi./
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(POP_TYPE_NONE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(pdMS_TO_TICKS(5000));
        abort();
    }

    /**
     * @brief Enable Wi-Fi Radar Detection
     */
    radar_start();
    ping_router_start(RADAR_PING_DATA_INTERVAL);
}
