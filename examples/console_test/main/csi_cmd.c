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
#include "cmd_system.h"
#include "argtable3/argtable3.h"
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

#include "esp_radar.h"
#include "app_priv.h"

static const char *TAG  = "csi_cmd";

extern float g_move_absolute_threshold;
extern float g_move_relative_threshold;
extern void wifi_csi_raw_cb(void *ctx, wifi_csi_info_t *info);

static struct {
    struct arg_int *len;
    struct arg_str *mac;
    struct arg_lit *output_raw_data;
    struct arg_end *end;
} csi_args;

static int wifi_cmd_csi(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &csi_args) != ESP_OK) {
        arg_print_errors(stderr, csi_args.end, argv[0]);
        return ESP_FAIL;
    }

    wifi_radar_config_t radar_config = {0};
    esp_wifi_radar_get_config(&radar_config);

    if (csi_args.len->count) {
        if (csi_args.len->ival[0] != 0 && csi_args.len->ival[0] != 128
                && csi_args.len->ival[0] != 256 && csi_args.len->ival[0] != 376
                && csi_args.len->ival[0] != 384 && csi_args.len->ival[0] != 612) {
            ESP_LOGE(TAG, "FAIL Filter CSI total bytes, 128 or 256 or 384");
            return ESP_FAIL;
        }

        radar_config.filter_len = csi_args.len->ival[0];
    }

    if (csi_args.mac->count) {
        if (!mac_str2hex(csi_args.mac->sval[0], radar_config.filter_mac)) {
            ESP_LOGE(TAG, "The format of the address is incorrect."
                     "Please enter the format as xx:xx:xx:xx:xx:xx");
            return ESP_FAIL;
        }
    }

    if (csi_args.output_raw_data->count) {
        radar_config.wifi_csi_raw_cb = wifi_csi_raw_cb;
    }

    esp_wifi_radar_set_config(&radar_config);

    return 0;
}

void cmd_register_csi(void)
{
    csi_args.len = arg_int0("l", "len", "len (0/128/256/376/384/612)>", "Filter CSI total bytes");
    csi_args.mac = arg_str0("m", "mac", "mac (xx:xx:xx:xx:xx:xx)>", "Filter device mac");
    csi_args.output_raw_data = arg_lit0("o", "output_raw_data", "Enable the output of raw csi data");
    csi_args.end = arg_end(1);

    const esp_console_cmd_t csi_cmd = {
        .command = "csi",
        .help = "CSI command",
        .hint = NULL,
        .func = &wifi_cmd_csi,
        .argtable = &csi_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&csi_cmd));
}

static struct {
    struct arg_str *move_absolute_threshold;
    struct arg_str *move_relative_threshold;
    struct arg_end *end;
} detect_args;

static int wifi_cmd_detect(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &detect_args) != ESP_OK) {
        arg_print_errors(stderr, detect_args.end, argv[0]);
        return ESP_FAIL;
    }

    if (detect_args.move_absolute_threshold->count) {
        float threshold = atof(detect_args.move_absolute_threshold->sval[0]);

        if (threshold > 0.0 && threshold < 1.0) {
            g_move_absolute_threshold = threshold;
        } else {
            ESP_LOGE(TAG, "If the setting fails, the absolute threshold range of human movement is: 0.0 ~ 1.0");
        }
    }

    if (detect_args.move_relative_threshold->count) {
        float threshold = atof(detect_args.move_relative_threshold->sval[0]);

        if (threshold > 0.0 && threshold < 1.0) {
            g_move_relative_threshold = threshold;
        } else {
            ESP_LOGE(TAG, "If the setting fails, the absolute threshold range of human movement is: 1.0 ~ 5.0");
        }
    }

    return ESP_OK;
}

void cmd_register_detect(void)
{
    detect_args.move_absolute_threshold = arg_str0("a", "move_absolute_threshold", "<0.0 ~ 1.0>", "Set the absolute threshold of human movement");
    detect_args.move_relative_threshold = arg_str0("r", "move_relative_threshold", "<1.0 ~ 5.0>", "Set the relative threshold of human movement");
    detect_args.end = arg_end(1);

    const esp_console_cmd_t detect_cmd = {
        .command = "detect",
        .help = "Detect command",
        .hint = NULL,
        .func = &wifi_cmd_detect,
        .argtable = &detect_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&detect_cmd));
}
