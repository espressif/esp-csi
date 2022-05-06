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
  * @brief Wi-Fi Radar data type
  *
  */
typedef struct {
    uint32_t time_spent;
    float amplitude_corr;
    float amplitude_corr_min;
    float amplitude_corr_max;
} wifi_radar_info_t;

#define CSI_LLFT_LEN           52
#define CSI_HT_LFT_LEN         56
#define CSI_STBC_HT_LFT_LEN    56
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl; /**< received packet radio metadata header of the CSI data */
    uint8_t mac[6];             /**< source MAC address of the CSI data */
    uint16_t raw_len;
    int8_t *raw_data;
    int16_t valid_len;
    int8_t valid_data[0];
} wifi_csi_filtered_info_t;

/**
  * @brief The RX callback function of Wi-Fi radar data.
  *
  *        Each time Wi-Fi radar data analysis, the callback function will be called.
  *
  * @param info Wi-Fi radar data received. The memory that it points to will be deallocated after callback function returns.
  * @param ctx context argument, passed to esp_wifi_radar_set_config() when registering callback function.
  *
  */
typedef void (*wifi_radar_cb_t)(const wifi_radar_info_t *info, void *ctx);


typedef void (*wifi_csi_filtered_cb_t)(const wifi_csi_filtered_info_t *info, void *ctx);

/**
  * @brief Wi-Fi radar configuration type
  */
typedef struct {
    uint8_t filter_mac[6];                 /**< Get the mac of the specified device, no filtering: [0xff:0xff:0xff:0xff:0xff:0xff] */
    uint16_t filter_len;                   /**< source MAC address of the CSI data, no filtering: 0 */
    wifi_radar_cb_t wifi_radar_cb;         /**< Register the callback function of Wi-Fi radar data */
    void *wifi_radar_cb_ctx;               /**< Context argument, passed to callback function of Wi-Fi radar */
    wifi_csi_filtered_cb_t wifi_csi_filtered_cb;         /**< Register the callback function of Wi-Fi CSI data */
    void *wifi_csi_cb_ctx;                 /**< Context argument, passed to callback function of Wi-Fi CSI */
    wifi_promiscuous_cb_t wifi_sniffer_cb; /**< The RX callback function in the promiscuous mode */
} wifi_radar_config_t;

/**
  * @brief Set Wi-Fi radar configuration
  *
  * @param config configuration
  *
  * return
  *    - ESP_OK
  *    - ESP_ERR_INVALID_ARG
  */
esp_err_t esp_wifi_radar_set_config(const wifi_radar_config_t *config);

/**
  * @brief Get Wi-Fi radar configuration
  *
  * @param config configuration
  *
  * return
  *    - ESP_OK
  *    - ESP_ERR_INVALID_ARG
  */
esp_err_t esp_wifi_radar_get_config(wifi_radar_config_t *config);

/**
  * @brief  Start WiFi radar according to current configuration
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_wifi_radar_start(void);

/**
  * @brief  Stop WiFi radar
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_wifi_radar_stop(void);

/**
  * @brief  Init WiFi radar
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_wifi_radar_init(void);

/**
  * @brief  Deinit Wi-Fi radar
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_wifi_radar_deinit(void);


typedef enum {
    RADAR_ACTION_UNKNOWN, 
    RADAR_ACTION_NONE, 
    RADAR_ACTION_SOMEONE, 
    RADAR_ACTION_STATIC, 
    RADAR_ACTION_MOVE, 
    RADAR_ACTION_FRONT, 
    RADAR_ACTION_AFTER, 
    RADAR_ACTION_LEFT, 
    RADAR_ACTION_RIGHT, 
    RADAR_ACTION_GO,
    RADAR_ACTION_JUMP, 
    RADAR_ACTION_SITDOWN, 
    RADAR_ACTION_STANDUP,
    RADAR_ACTION_CLIMBUP, 
    RADAR_ACTION_WAVE, 
    RADAR_ACTION_APPLAUSE,
    RADAR_ACTION_NUM,
} radar_action_type_t;

esp_err_t esp_radar_action_calibrate_start(radar_action_type_t action);

esp_err_t esp_radar_action_calibrate_stop(radar_action_type_t action);

#ifdef __cplusplus
}
#endif
