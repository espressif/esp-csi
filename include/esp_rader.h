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

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/param.h>
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
  * @brief Wi-Fi Rader data type
  *
  */
typedef struct {
    uint32_t time_start;     /**< Start time detection. unit: microsecond */
    uint32_t time_end;       /**< Stop time detection. unit: microsecond */
    int8_t rssi_avg;         /**< Received Signal Strength Indicator(RSSI) of packet. unit: dBm */
    float rssi_std;          /**< Received Signal Strength Indicator(RSSI) of packet. unit: dBm */
    float amplitude_corr[3]; /**< Average value between sub-carriers */
    float amplitude_std[3];  /**< Standard deviation between sub-carriers. 
                                  Subcarrier: lltf(6 ~ 31,33 ~ 58), htltf(66 ~ 122), stbc_htltf(123 ~ 191)*/
} wifi_rader_info_t;

/**
  * @brief The RX callback function of Wi-Fi rader data.
  *
  *        Each time Wi-Fi radar data analysis, the callback function will be called.
  *
  * @param info Wi-Fi rader data received. The memory that it points to will be deallocated after callback function returns.
  * @param ctx context argument, passed to esp_wifi_rader_set_config() when registering callback function.
  *
  */
typedef void (*wifi_rader_cb_t)(const wifi_rader_info_t *info, void *ctx);

/**
  * @brief Wi-Fi rader configuration type
  */
typedef struct {
    uint8_t filter_mac[6];                 /**< Get the mac of the specified device, no filtering: [0xff:0xff:0xff:0xff:0xff:0xff] */
    uint16_t filter_len;                   /**< source MAC address of the CSI data, no filtering: 0 */
    wifi_rader_cb_t wifi_rader_cb;         /**< Register the callback function of Wi-Fi rader data */
    wifi_rader_cb_t wifi_rader_cb_ctx;     /**< Context argument, passed to callback function of Wi-Fi rader */
    wifi_csi_cb_t wifi_csi_raw_cb;         /**< Register the callback function of Wi-Fi CSI data */
    wifi_rader_cb_t wifi_csi_cb_ctx;       /**< Context argument, passed to callback function of Wi-Fi CSI */
    wifi_promiscuous_cb_t wifi_sniffer_cb; /**< The RX callback function in the promiscuous mode */
} wifi_rader_config_t;

/**
  * @brief Set Wi-Fi Rader configuration
  *
  * @param config configuration
  *
  * return
  *    - ESP_OK
  *    - ESP_ERR_INVALID_ARG
  */
esp_err_t esp_wifi_rader_set_config(const wifi_rader_config_t *config);

/**
  * @brief Get Wi-Fi Rader configuration
  *
  * @param config configuration
  *
  * return
  *    - ESP_OK
  *    - ESP_ERR_INVALID_ARG
  */
esp_err_t esp_wifi_rader_get_config(wifi_rader_config_t *config);

/**
  * @brief  Start WiFi rader according to current configuration
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_wifi_rader_start(void);

/**
  * @brief  Stop WiFi rader
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_wifi_rader_stop(void);

/**
  * @brief  Init WiFi rader
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_wifi_rader_init(void);

/**
  * @brief  Deinit Wi-Fi rader
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_wifi_rader_deinit(void);

#ifdef __cplusplus
}
#endif
