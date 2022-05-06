// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_console.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_DISCONNECTED_BIT BIT1

static bool s_reconnect = true;
static const char *TAG  = "wifi_cmd";

static EventGroupHandle_t s_wifi_event_group;

static bool mac_str2hex(const char *mac_str, uint8_t *mac_hex)
{
    uint32_t mac_data[6] = {0};
    int ret = sscanf(mac_str, MACSTR, mac_data, mac_data + 1, mac_data + 2,
                     mac_data + 3, mac_data + 4, mac_data + 5);

    for (int i = 0; i < 6; i++) {
        mac_hex[i] = mac_data[i];
    }

    return ret == 6 ? true : false;
}

/* Event handler for catching system events */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_reconnect) {
            ESP_LOGI(TAG, "sta disconnect, s_reconnect...");
            esp_wifi_connect();
        } else {
            ESP_LOGI(TAG, "sta disconnect");
        }

        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
        ESP_LOGI(TAG, "Connected to %s (bssid: "MACSTR", channel: %d)", event->ssid,
                 MAC2STR(event->bssid), event->channel);
    }  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "STA Connecting to the AP again...");
    }
}

typedef struct {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} wifi_args_t;

static wifi_args_t sta_args;
static wifi_args_t ap_args;

static int wifi_cmd_sta(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &sta_args) != ESP_OK) {
        arg_print_errors(stderr, sta_args.end, argv[0]);
        return ESP_FAIL;
    }

    const char *ssid     = sta_args.ssid->sval[0];
    const char *password = sta_args.password->sval[0];
    wifi_config_t wifi_config = { 0 };
    static esp_netif_t *s_netif_sta = NULL;

    if(!s_wifi_event_group){
        s_wifi_event_group = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    }

    if (!s_netif_sta) {
        s_netif_sta = esp_netif_create_default_wifi_sta();
    }

    strlcpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));

    if (password) {
        strlcpy((char *) wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }

    ESP_LOGI(TAG, "sta connecting to '%s'", ssid);

    int bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, false);

    if (bits & WIFI_CONNECTED_BIT) {
        s_reconnect = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        xEventGroupWaitBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT, false, true, portTICK_RATE_MS);
    }

    s_reconnect = true;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    esp_wifi_connect();

    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, pdMS_TO_TICKS(5000));

    return ESP_OK;
}

static int wifi_cmd_ap(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &ap_args) != ESP_OK) {
        arg_print_errors(stderr, ap_args.end, argv[0]);
        return ESP_FAIL;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .max_connection = 4,
            .password = "",
            .channel  = 11,
        },
    };

    const char *ssid = ap_args.ssid->sval[0];
    const char *password = ap_args.password->sval[0];
    static esp_netif_t *s_netif_ap = NULL;

    if(!s_wifi_event_group){
        s_wifi_event_group = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    }

    if (!s_netif_ap) {
        s_netif_ap = esp_netif_create_default_wifi_ap();
    }

    s_reconnect = false;
    strlcpy((char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));

    if (password && strlen(password)) {
        if (strlen(password) < 8) {
            s_reconnect = true;
            ESP_LOGE(TAG, "password less than 8");
            return ESP_FAIL;
        }

        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        strlcpy((char *) wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));

    ESP_LOGI(TAG, "Starting SoftAP SSID: %s, Password: %s", ssid, password);

    return ESP_OK;
}

static int wifi_cmd_query(int argc, char **argv)
{
    wifi_config_t cfg;
    wifi_mode_t mode;

    esp_wifi_get_mode(&mode);

    if (WIFI_MODE_AP == mode) {
        esp_wifi_get_config(WIFI_IF_AP, &cfg);
        ESP_LOGI(TAG, "AP mode, %s %s", cfg.ap.ssid, cfg.ap.password);
    } else if (WIFI_MODE_STA == mode) {
        int bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, 0, 1, 0);

        if (bits & WIFI_CONNECTED_BIT) {
            esp_wifi_get_config(WIFI_IF_STA, &cfg);
            ESP_LOGI(TAG, "sta mode, connected %s", cfg.ap.ssid);
        } else {
            ESP_LOGI(TAG, "sta mode, disconnected");
        }
    } else {
        ESP_LOGI(TAG, "NULL mode");
        return 0;
    }

    return 0;
}

void cmd_register_wifi(void)
{
    sta_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    sta_args.password = arg_str0(NULL, NULL, "<password>", "password of AP");
    sta_args.end = arg_end(2);

    const esp_console_cmd_t sta_cmd = {
        .command = "sta",
        .help = "WiFi is station mode, join specified soft-AP",
        .hint = NULL,
        .func = &wifi_cmd_sta,
        .argtable = &sta_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&sta_cmd));

    ap_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    ap_args.password = arg_str0(NULL, NULL, "<password>", "password of AP");
    ap_args.end = arg_end(2);

    const esp_console_cmd_t ap_cmd = {
        .command = "ap",
        .help = "AP mode, configure ssid and password",
        .hint = NULL,
        .func = &wifi_cmd_ap,
        .argtable = &ap_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&ap_cmd));

    const esp_console_cmd_t query_cmd = {
        .command = "query",
        .help = "query WiFi info",
        .hint = NULL,
        .func = &wifi_cmd_query,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&query_cmd));
}


static struct {
    struct arg_str *country_code;
    struct arg_int *channel;
    struct arg_int *channel_second;
    struct arg_str *ssid;
    struct arg_str *bssid;
    struct arg_str *password;
    struct arg_int *tx_power;
    struct arg_lit *info;
    struct arg_int *rate;
    struct arg_int *bandwidth;
    struct arg_int *protocol;
    struct arg_end *end;
} wifi_config_args;

/**
 * @brief  A function which implements wifi config command.
 */
static int wifi_config_func(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &wifi_config_args) != ESP_OK) {
        arg_print_errors(stderr, wifi_config_args.end, argv[0]);
        return ESP_FAIL;
    }

    if(!s_wifi_event_group) {
        s_wifi_event_group = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    }

    esp_err_t ret = ESP_OK;
    wifi_config_t wifi_config = {0x0};
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    static esp_netif_t *s_netif_sta = NULL;

    if (!s_netif_sta) {
        s_netif_sta = esp_netif_create_default_wifi_sta();
    }

    if (wifi_config_args.ssid->count) {
        strcpy((char *)wifi_config.sta.ssid, wifi_config_args.ssid->sval[0]);
    }

    if (wifi_config_args.password->count) {
        strcpy((char *)wifi_config.sta.password, wifi_config_args.password->sval[0]);
    }

    if (wifi_config_args.bssid->count) {
        if(!mac_str2hex(wifi_config_args.bssid->sval[0], wifi_config.sta.bssid)) {
            ESP_LOGE(TAG, "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");
            return ESP_ERR_INVALID_ARG;
        }
    }

    if (strlen((char *)wifi_config.sta.ssid)) {
        int bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, false);

        if (bits & WIFI_CONNECTED_BIT) {
            s_reconnect = false;
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            ESP_ERROR_CHECK(esp_wifi_disconnect());
            xEventGroupWaitBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT, false, true, portTICK_RATE_MS);
        }

        s_reconnect = true;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    //     esp_err_t esp_wifi_internal_set_fix_rate(wifi_interface_t ifx, bool en, wifi_phy_rate_t rate);
    //     esp_wifi_internal_set_fix_rate(ESP_IF_WIFI_STA, true, WIFI_PHY_RATE_48M);
    // ESP_LOGW(TAG, "------- <%s, %d> ------", __func__, __LINE__);

        // ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20));

        // ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G));

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_connect();
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, pdMS_TO_TICKS(5000));
    }

    if (wifi_config_args.country_code->count) {
        wifi_country_t country = {0};
        const char *country_code = wifi_config_args.country_code->sval[0];

        if (!strcasecmp(country_code, "US")) {
            strcpy(country.cc, "US");
            country.schan = 1;
            country.nchan = 11;
        } else if (!strcasecmp(country_code, "JP")) {
            strcpy(country.cc, "JP");
            country.schan = 1;
            country.nchan = 14;
        }  else if (!strcasecmp(country_code, "CN")) {
            strcpy(country.cc, "CN");
            country.schan = 1;
            country.nchan = 13;
        } else {
            return ESP_ERR_INVALID_ARG;
        }

        ret = esp_wifi_set_country(&country);
    }

    if (wifi_config_args.channel->count) {
        wifi_config.sta.channel = wifi_config_args.channel->ival[0];
        wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;

        if (wifi_config_args.channel_second->count < 3) {
            second = wifi_config_args.channel_second->ival[0];
        }

        if (wifi_config.sta.channel > 0 && wifi_config.sta.channel <= 14) {
            ret = esp_wifi_set_channel(wifi_config.sta.channel, second);
            
            if(ret == ESP_OK) {
                ESP_LOGI(TAG, "Set Channel, channel: %d", wifi_config.sta.channel);
            } else {
                ESP_LOGE(TAG, "<%s> esp_wifi_set_channel", esp_err_to_name(ret));   
                return ESP_ERR_INVALID_ARG;
            }
        } else {
            return ESP_ERR_INVALID_ARG;
        }
    }

    if (wifi_config_args.tx_power->count) {
        esp_wifi_set_max_tx_power(wifi_config_args.tx_power->ival[0]);
    }

    if (wifi_config_args.info->count) {
        int8_t rx_power           = 0;
        wifi_country_t country    = {0};
        uint8_t primary           = 0;
        wifi_second_chan_t second = 0;
        wifi_mode_t mode          = 0;
        wifi_bandwidth_t bandwidth = 0;

        esp_wifi_get_channel(&primary, &second);
        esp_wifi_get_max_tx_power(&rx_power);
        esp_wifi_get_country(&country);
        esp_wifi_get_mode(&mode);
        esp_wifi_get_bandwidth(mode - 1, &bandwidth);

        country.cc[2] = '\0';
        ESP_LOGI(TAG, "tx_power: %d, country: %s, channel: %d/%d, mode: %d, bandwidth: %d",
                 rx_power, country.cc, primary, second, mode, bandwidth);
    }

    if (wifi_config_args.bandwidth->count) {
        ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, wifi_config_args.bandwidth->ival[0]));
    }

    if (wifi_config_args.rate->count) {
        extern esp_err_t esp_wifi_internal_set_fix_rate(wifi_interface_t ifx, bool en, wifi_phy_rate_t rate);
        ESP_ERROR_CHECK(esp_wifi_internal_set_fix_rate(ESP_IF_WIFI_STA, true, wifi_config_args.rate->ival[0]));
    }

    if (wifi_config_args.protocol->count) {
        ESP_ERROR_CHECK(esp_wifi_set_protocol(ESP_IF_WIFI_STA, wifi_config_args.protocol->ival[0]));
    }

    return ret;
}

/**
 * @brief  Register wifi config command.
 */
void cmd_register_wifi_config()
{
    wifi_config_args.ssid     = arg_str0("s", "ssid", "<ssid>", "SSID of router");
    wifi_config_args.password = arg_str0("p", "password", "<password>", "Password of router");
    wifi_config_args.bssid    = arg_str0("b", "bssid", "<bssid (xx:xx:xx:xx:xx:xx)>", "BSSID of router");
    wifi_config_args.channel  = arg_int0("c", "channel", "<channel (1 ~ 14)>", "Set primary channel");
    wifi_config_args.channel_second = arg_int0("c", "channel_second", "<channel_second (0, 1, 2)>", "Set second channel");
    wifi_config_args.country_code = arg_str0("C", "country_code", "<country_code ('CN', 'JP, 'US')>", "Set the current country code");
    wifi_config_args.tx_power = arg_int0("t", "tx_power", "<power (8 ~ 84)>", "Set maximum transmitting power after WiFi start.");
    wifi_config_args.rate     = arg_int0("r", "rate", "<MCS>", "Set the bandwidth of ESP32 specified interface");
    wifi_config_args.bandwidth = arg_int0("w", "bandwidth", "<bandwidth(1: HT20, 2: HT40)>", "Set the bandwidth of ESP32 specified interface");
    wifi_config_args.protocol = arg_int0("P", "protocol", "<bgn(1,3,7)>", "Set protocol type of specified interface");
    wifi_config_args.info     = arg_lit0("i", "info", "Get Wi-Fi configuration information");
    wifi_config_args.end      = arg_end(9);

    const esp_console_cmd_t cmd = {
        .command = "wifi_config",
        .help = "Set the configuration of the ESP32 STA",
        .hint = NULL,
        .func = &wifi_config_func,
        .argtable = &wifi_config_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}


static struct {
    struct arg_int *rssi;
    struct arg_str *ssid;
    struct arg_str *bssid;
    struct arg_int *passive;
    struct arg_end *end;
} wifi_scan_args;

/**
 * @brief  A function which implements wifi scan command.
 */
static esp_err_t wifi_scan_func(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &wifi_scan_args) != ESP_OK) {
        arg_print_errors(stderr, wifi_scan_args.end, argv[0]);
        return ESP_FAIL;
    }

    int8_t filter_rssi             = -120;
    uint16_t ap_number             = 0;
    uint8_t bssid[6]               = {0x0};
    uint8_t channel                = 1;
    wifi_second_chan_t second      = 0;
    wifi_ap_record_t ap_record     = {0x0};
    wifi_scan_config_t scan_config = {
        .show_hidden = true,
        .scan_type   = WIFI_SCAN_TYPE_ACTIVE,
    };

    ESP_ERROR_CHECK(esp_wifi_get_channel(&channel, &second));
    ESP_ERROR_CHECK(esp_wifi_disconnect());

    if (wifi_scan_args.passive->count) {
        scan_config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
        scan_config.scan_time.passive = wifi_scan_args.passive->ival[0];
    }

    if (wifi_scan_args.ssid->count) {
        scan_config.ssid = (uint8_t *)wifi_scan_args.ssid->sval[0];
    }

    if (wifi_scan_args.bssid->count) {
        if(!mac_str2hex(wifi_scan_args.bssid->sval[0], bssid)) {
            ESP_LOGE(TAG, "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");
            return ESP_ERR_INVALID_ARG;
        }
        
        scan_config.bssid = bssid;
    }

    if (wifi_scan_args.rssi->count) {
        filter_rssi = wifi_scan_args.rssi->ival[0];
        ESP_LOGW(TAG, "filter_rssi: %d", filter_rssi);
    }

    esp_wifi_scan_stop();

    int retry_count = 20;

    do {
        esp_wifi_disconnect();
        esp_wifi_scan_start(&scan_config, true);
        esp_wifi_scan_get_ap_num(&ap_number);
    } while (ap_number <= 0 && --retry_count);

    if(ap_number <= 0){
        ESP_LOGE(TAG, "esp_wifi_scan_get_ap_num");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "Get number of APs found, number: %d", ap_number);

    for (int i = 0; i < ap_number; i++) {
        memset(&ap_record, 0, sizeof(wifi_ap_record_t));

        if (ap_record.rssi < filter_rssi) {
            continue;
        }

        ESP_LOGI(TAG, "Router, ssid: %s, bssid: " MACSTR ", channel: %u, rssi: %d",
                 ap_record.ssid, MAC2STR(ap_record.bssid),
                 ap_record.primary, ap_record.rssi);
    }


    if (channel > 0 && channel < 13) {
        ESP_ERROR_CHECK(esp_wifi_set_channel(channel, second));
    }

    return ESP_OK;
}

/**
 * @brief  Register wifi scan command.
 */
void cmd_register_wifi_scan()
{
    wifi_scan_args.rssi    = arg_int0("r", "rssi", "<rssi (-120 ~ 0)>", "Filter device uses RSSI");
    wifi_scan_args.ssid    = arg_str0("s", "ssid", "<ssid>", "Filter device uses SSID");
    wifi_scan_args.bssid   = arg_str0("b", "bssid", "<bssid (xx:xx:xx:xx:xx:xx)>", "Filter device uses AP's MAC");
    wifi_scan_args.passive = arg_int0("p", "passive", "<time (ms)>", "Passive scan time per channel");
    wifi_scan_args.end     = arg_end(5);

    const esp_console_cmd_t cmd = {
        .command  = "wifi_scan",
        .help     = "Wi-Fi is station mode, start scan ap",
        .hint     = NULL,
        .func     = &wifi_scan_func,
        .argtable = &wifi_scan_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}