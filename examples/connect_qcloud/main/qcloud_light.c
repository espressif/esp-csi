/* LED Light Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <assert.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_qcloud_log.h"
#include "esp_qcloud_console.h"
#include "esp_qcloud_storage.h"
#include "esp_qcloud_iothub.h"
#include "esp_qcloud_prov.h"
#include "esp_ota_ops.h"

#include "esp_radar.h"

#include "light_driver.h"
#include "qcloud_light.h"

#include "utils.h"

#if defined (CONFIG_LIGHT_TYPE_C3_lighting) || defined (CONFIG_LIGHT_TYPE_S2_lighting)
#define SUPPORT_HIGH_POWER 1
#endif


static const char *TAG = "qcloud_light";

/* Callback to handle commands received from the QCloud cloud */
static esp_err_t light_get_param(const char *id, esp_qcloud_param_val_t *val)
{
    char tmp_buf[32] = {0};
    wifi_radar_data_t wifi_radar_data = {0};
    wifi_radar_get_data(&wifi_radar_data);

    if (!strcmp(id, "power_switch")) {
        val->b = light_driver_get_switch();
    } else if (!strcmp(id, "value")) {
        val->i = light_driver_get_value();
    } else if (!strcmp(id, "hue")) {
        val->i = light_driver_get_hue();
    } else if (!strcmp(id, "saturation")) {
        val->i = light_driver_get_saturation();
    } else if (!strcmp(id, "wifi_rader_data")) {
        val->s = "[]";
    } else if (!strcmp(id, "wifi_rader_adjust_threshold")) {
        val->b = wifi_radar_data.threshold_adjust;
    } else if (!strcmp(id, "wifi_rader_human_move")) {
        val->f = wifi_radar_data.move_absolute_threshold;
    } else if (!strcmp(id, "wifi_rader_human_detect")) {
        val->f = wifi_radar_data.threshold_human_detect;
    } else if (!strcmp(id, "wifi_rader_room_status")) {
        val->b = wifi_radar_data.room_status;
    } else if (!strcmp(id, "wifi_rader_awake_count")) {
        val->i = wifi_radar_data.awake_count;
    } else if (!strcmp(id, "wifi_rader_status")) {
        val->b = wifi_radar_data.status_enable;
    } else if (!strcmp(id, "wifi_rader_data_mac")) {
        val->s = tmp_buf;
        wifi_radar_config_t radar_config = {0};
        esp_wifi_radar_get_config(&radar_config);
        sprintf(tmp_buf, MACSTR, MAC2STR(radar_config.filter_mac));
    } else if (!strcmp(id, "mac")) {
        val->s = tmp_buf;
        uint8_t mac[6];
        esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
        sprintf(tmp_buf, MACSTR, MAC2STR(mac));
    } else if (!strcmp(id, "ap_bssid")) {
        val->s = tmp_buf;
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        sprintf(tmp_buf, MACSTR, MAC2STR(ap_info.bssid));
    }

    ESP_LOGI(TAG, "Report id: %s, val: %d", id, val->i);

    return ESP_OK;
}

/* Callback to handle commands received from the QCloud cloud */
static esp_err_t light_set_param(const char *id, const esp_qcloud_param_val_t *val)
{
    esp_err_t err = ESP_OK;
    wifi_radar_data_t wifi_radar_data = {0};
    wifi_radar_get_data(&wifi_radar_data);

    ESP_LOGI(TAG, "Received id: %s, val: %d", id, val->i);

    if (!strcmp(id, "power_switch")) {
        err = light_driver_set_switch(val->b);
    } else if (!strcmp(id, "value")) {
        err = light_driver_set_value(val->i);
    } else if (!strcmp(id, "hue")) {
        err = light_driver_set_hue(val->i);
    } else if (!strcmp(id, "saturation")) {
        err = light_driver_set_saturation(val->i);
    } else if (!strcmp(id, "wifi_rader_adjust_threshold")) {
        wifi_radar_data.threshold_adjust = val->b;

        if (wifi_radar_data.threshold_adjust) {
            wifi_radar_data.awake_count = 0;
#ifdef SUPPORT_HIGH_POWER
            light_driver_set_hsv(0,0,100);
#endif
            light_driver_breath_start(255, 200, 0);
        } else {
            wifi_radar_data.move_absolute_threshold += wifi_radar_data.move_absolute_threshold * 0.1;
            wifi_radar_data.move_relative_threshold += wifi_radar_data.move_relative_threshold * 0.1;
            wifi_radar_data.threshold_human_detect -= wifi_radar_data.threshold_human_detect * 0.1;
            light_driver_breath_stop();
#ifdef SUPPORT_HIGH_POWER
            light_driver_set_ctb(50,100);
#endif
            esp_qcloud_storage_set("move_absolute", &wifi_radar_data.move_absolute_threshold, sizeof(float));
            esp_qcloud_storage_set("move_relative", &wifi_radar_data.move_relative_threshold, sizeof(float));
            esp_qcloud_storage_set("human_detect", &wifi_radar_data.threshold_human_detect, sizeof(float));
        }
    } else if (!strcmp(id, "wifi_rader_human_move")) {
        wifi_radar_data.move_absolute_threshold = val->f;
        esp_qcloud_storage_set("move_absolute", &wifi_radar_data.move_absolute_threshold, sizeof(float));
    } else if (!strcmp(id, "wifi_rader_human_detect")) {
        wifi_radar_data.threshold_human_detect = val->f;
        esp_qcloud_storage_set("human_detect", &wifi_radar_data.threshold_human_detect, sizeof(float));
    } else if (!strcmp(id, "wifi_rader_status")) {
        wifi_radar_data.status_enable = val->b;
    } else if (!strcmp(id, "wifi_rader_data_mac")) {
        wifi_radar_config_t radar_config = {0};
        esp_wifi_radar_get_config(&radar_config);
        mac_str2hex(val->s, radar_config.filter_mac);
        esp_wifi_radar_set_config(&radar_config);

        esp_qcloud_storage_set("filter_mac", radar_config.filter_mac,
                               sizeof(radar_config.filter_mac));
    } else {
        ESP_LOGW(TAG, "This parameter is not supported");
    }

    wifi_radar_set_data(&wifi_radar_data);

    return err;
}

/* Event handler for catching QCloud events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int event_id, void *event_data)
{
    switch (event_id) {
        case QCLOUD_EVENT_IOTHUB_INIT_DONE:
            esp_qcloud_iothub_report_device_info();
            ESP_LOGI(TAG, "QCloud Initialised");
            break;

        case QCLOUD_EVENT_IOTHUB_BOND_DEVICE:
            ESP_LOGI(TAG, "Device binding successful");
            break;

        case QCLOUD_EVENT_IOTHUB_UNBOND_DEVICE:
            ESP_LOGW(TAG, "Device unbound with iothub");
            esp_qcloud_storage_erase(CONFIG_QCLOUD_NVS_NAMESPACE);
            esp_restart();
            break;

        case QCLOUD_EVENT_IOTHUB_BIND_EXCEPTION:
            ESP_LOGW(TAG, "Device bind fail");
            esp_qcloud_storage_erase(CONFIG_QCLOUD_NVS_NAMESPACE);
            esp_restart();
            break;

        case QCLOUD_EVENT_IOTHUB_RECEIVE_STATUS:
            ESP_LOGI(TAG, "receive status message: %s", (char *)event_data);
            break;

        default:
            ESP_LOGW(TAG, "Unhandled QCloud Event: %d", event_id);
    }
}

static esp_err_t get_wifi_config(wifi_config_t *wifi_cfg, uint32_t wait_ms)
{
    ESP_QCLOUD_PARAM_CHECK(wifi_cfg);

    if (esp_qcloud_storage_get("wifi_config", wifi_cfg, sizeof(wifi_config_t)) == ESP_OK) {
#if CONFIG_BT_ENABLE
#include "esp_bt.h"
        esp_bt_mem_release(ESP_BT_MODE_BTDM);
#endif
        return ESP_OK;
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    /**< The yellow light flashes to indicate that the device enters the state of configuring the network */
    light_driver_breath_start(128, 128, 0); /**< yellow blink */

    /**< Note: Smartconfig and softapconfig working at the same time will affect the configure network performance */

#ifdef CONFIG_LIGHT_PROVISIONING_SOFTAPCONFIG
    char softap_ssid[32 + 1] = CONFIG_LIGHT_PROVISIONING_SOFTAPCONFIG_SSID;
    // uint8_t mac[6] = {0};
    // ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
    // sprintf(softap_ssid, "tcloud_%s_%02x%02x", light_driver_get_type(), mac[4], mac[5]);

    esp_qcloud_prov_softapconfig_start(SOFTAPCONFIG_TYPE_ESPRESSIF_TENCENT,
                                       softap_ssid,
                                       CONFIG_LIGHT_PROVISIONING_SOFTAPCONFIG_PASSWORD);
    esp_qcloud_prov_print_wechat_qr(softap_ssid, "softap");
#endif

#ifdef CONFIG_LIGHT_PROVISIONING_SMARTCONFIG
    esp_qcloud_prov_smartconfig_start(SC_TYPE_ESPTOUCH_AIRKISS);
#endif

#ifdef CONFIG_LIGHT_PROVISIONING_BLECONFIG
    char local_name[32 + 1] = CONFIG_LIGHT_PROVISIONING_BLECONFIG_NAME;
    esp_qcloud_prov_bleconfig_start(BLECONFIG_TYPE_ESPRESSIF_TENCENT, local_name);
#endif

    ESP_ERROR_CHECK(esp_qcloud_prov_wait(wifi_cfg, wait_ms));

#ifdef CONFIG_LIGHT_PROVISIONING_SMARTCONFIG
    esp_qcloud_prov_smartconfig_stop();
#endif

#ifdef CONFIG_LIGHT_PROVISIONING_SOFTAPCONFIG
    esp_qcloud_prov_softapconfig_stop();
#endif

    /**< Store the configure of the device */
    esp_qcloud_storage_set("wifi_config", wifi_cfg, sizeof(wifi_config_t));

    /**< Configure the network successfully to stop the light flashing */
    light_driver_breath_stop(); /**< stop blink */

    return ESP_OK;
}

void qcloud_light_report_status(void *avg)
{
    wifi_radar_data_t wifi_radar_data = {0};

    wifi_radar_get_data(&wifi_radar_data);

    if (!wifi_radar_data.awake_count) {
        return;
    }

    esp_qcloud_method_t *report = esp_qcloud_iothub_create_report();

    esp_qcloud_iothub_param_add_int(report, "wifi_rader_awake_count", wifi_radar_data.awake_count);
    esp_qcloud_iothub_param_add_float(report, "wifi_rader_room_status", wifi_radar_data.room_status);

    esp_qcloud_iothub_post_method(report);
    esp_qcloud_iothub_destroy_report(report);

    ESP_LOGW(TAG, "awake_count: %d", wifi_radar_data.awake_count);

    wifi_radar_data.awake_count = 0;

    wifi_radar_set_data(&wifi_radar_data);
}

esp_err_t qcloud_light_init(void)
{
    /**
     * @brief Add debug function, you can use serial command and remote debugging.
     */
    esp_qcloud_log_config_t log_config = {
        .log_level_uart = ESP_LOG_INFO,
    };
    ESP_ERROR_CHECK(esp_qcloud_log_init(&log_config));
    /**
     * @brief Set log level
     * @note  This function can not raise log level above the level set using
     * CONFIG_LOG_DEFAULT_LEVEL setting in menuconfig.
     */
    esp_log_level_set("*", ESP_LOG_VERBOSE);

#ifdef CONFIG_LIGHT_DEBUG
    ESP_ERROR_CHECK(esp_qcloud_console_init());
    esp_qcloud_print_system_info(10000);
#endif /**< CONFIG_LIGHT_DEBUG */

    /**
     * @brief Initialize Application specific hardware drivers and set initial state.
     */
    light_driver_config_t driver_cfg = COFNIG_LIGHT_TYPE_DEFAULT();
    ESP_ERROR_CHECK(light_driver_init(&driver_cfg));

    /**< Continuous power off and restart more than five times to reset the device */
    if (esp_qcloud_reboot_unbroken_count() >= CONFIG_LIGHT_REBOOT_UNBROKEN_COUNT_RESET) {
        ESP_LOGW(TAG, "Erase information saved in flash");
        esp_qcloud_storage_erase(CONFIG_QCLOUD_NVS_NAMESPACE);
    } else if (esp_qcloud_reboot_is_exception(false)) {
        ESP_LOGE(TAG, "The device has been restarted abnormally");
#ifdef CONFIG_LIGHT_DEBUG
        light_driver_breath_start(255, 0, 0); /**< red blink */
#else
        ESP_ERROR_CHECK(light_driver_set_switch(true));
#endif /**< CONFIG_LIGHT_DEBUG */
    } else {
        ESP_ERROR_CHECK(light_driver_set_switch(true));
    }

    /*
     * @breif Create a device through the server and obtain configuration parameters
     *        server: https://console.cloud.tencent.com/iotexplorer
     */
    /**< Create and configure device authentication information */
    ESP_ERROR_CHECK(esp_qcloud_create_device());
    /**< Configure the version of the device, and use this information to determine whether to OTA */
    const esp_app_desc_t *app_desc_ptr = esp_ota_get_app_description();
    assert(app_desc_ptr);
    ESP_ERROR_CHECK(esp_qcloud_device_add_fw_version(app_desc_ptr->version));

    /**< Register the properties of the device */
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("power_switch", QCLOUD_VAL_TYPE_BOOLEAN));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("saturation", QCLOUD_VAL_TYPE_INTEGER));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("value", QCLOUD_VAL_TYPE_INTEGER));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("hue", QCLOUD_VAL_TYPE_INTEGER));

    ESP_ERROR_CHECK(esp_qcloud_device_add_property("wifi_rader_data", QCLOUD_VAL_TYPE_STRING));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("wifi_rader_adjust_threshold", QCLOUD_VAL_TYPE_BOOLEAN));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("wifi_rader_room_status", QCLOUD_VAL_TYPE_BOOLEAN));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("wifi_rader_awake_count", QCLOUD_VAL_TYPE_INTEGER));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("wifi_rader_status", QCLOUD_VAL_TYPE_BOOLEAN));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("wifi_rader_data_mac", QCLOUD_VAL_TYPE_STRING));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("mac", QCLOUD_VAL_TYPE_STRING));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("ap_bssid", QCLOUD_VAL_TYPE_STRING));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("wifi_rader_human_move", QCLOUD_VAL_TYPE_FLOAT));
    ESP_ERROR_CHECK(esp_qcloud_device_add_property("wifi_rader_human_detect", QCLOUD_VAL_TYPE_FLOAT));


    /**< The processing function of the communication between the device and the server */
    ESP_ERROR_CHECK(esp_qcloud_device_add_property_cb(light_get_param, light_set_param));

    /**
     * @brief Initialize Wi-Fi.
     */
    ESP_ERROR_CHECK(esp_qcloud_wifi_init());
    ESP_ERROR_CHECK(esp_event_handler_register(QCLOUD_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /**
     * @brief Get the router configuration
     */
    wifi_config_t wifi_cfg = {0};
    ESP_ERROR_CHECK(get_wifi_config(&wifi_cfg, portMAX_DELAY));

    /**
     * @brief Connect to router
     */
    ESP_ERROR_CHECK(esp_qcloud_wifi_start(&wifi_cfg));
    ESP_ERROR_CHECK(esp_qcloud_timesync_start());

    /**
     * @brief Connect to Tencent Cloud Iothub
     */
    ESP_ERROR_CHECK(esp_qcloud_iothub_init());
    ESP_ERROR_CHECK(esp_qcloud_iothub_start());
    ESP_ERROR_CHECK(esp_qcloud_iothub_ota_enable());

    TimerHandle_t timer = xTimerCreate("report_status", 10000 / portTICK_RATE_MS,
                                       true, NULL, qcloud_light_report_status);
    xTimerStart(timer, 0);

#ifdef SUPPORT_HIGH_POWER
    ESP_ERROR_CHECK(light_driver_set_ctb(50, 100));
#endif
    return ESP_OK;
}
