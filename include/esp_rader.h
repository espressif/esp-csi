// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
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

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/param.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    uint32_t time_start;     /**< Timer spent getting information. unit: microsecond */
    uint32_t time_end;
    int8_t rssi_avg;         /**< Received Signal Strength Indicator(RSSI) of packet. unit: dBm */
    float rssi_std;          /**< Received Signal Strength Indicator(RSSI) of packet. unit: dBm */
    float amplitude_corr[3]; /**< Average value of correlation coefficient between sub-carriers */
    float amplitude_std[3];  /**< standard deviation of correlation coefficient between sub-carriers. 
                                  subcarrier: lltf(6 ~ 31,33 ~ 58), htltf(66 ~ 122), stbc_htltf(123 ~ 191)*/
} wifi_rader_info_t;

typedef void (*wifi_rader_cb_t)(const wifi_rader_info_t *info, void *ctx);

typedef struct {
    uint8_t filer_mac[6];  /**< source MAC address of the CSI data */
    // uint16_t pack_num;  /**< Nuber of packets received during analysis */
    wifi_rader_cb_t wifi_rader_cb;
    wifi_rader_cb_t wifi_rader_cb_ctx;
} wifi_rader_config_t;


esp_err_t esp_wifi_rader_set_config(const wifi_rader_config_t *config);
esp_err_t esp_wifi_rader_get_config(wifi_rader_config_t *config);

esp_err_t esp_wifi_rader_start();
esp_err_t esp_wifi_rader_stop();

esp_err_t esp_wifi_rader_init();
esp_err_t esp_wifi_rader_deinit();

esp_err_t esp_wifi_rader_ping_start();
esp_err_t esp_wifi_rader_ping_stop();

#ifdef __cplusplus
}
#endif
