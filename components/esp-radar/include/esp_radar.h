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
 */
typedef struct {
    float waveform_jitter;  /**< Jitter of the radar waveform, 用于检测人体移动 */
    float waveform_wander;  /**< Wander of the radar waveform，用于检测人体存在 */
} wifi_radar_info_t;

/**
 * @brief Channel state information(CSI) configuration type
 */
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl;     /**< received packet radio metadata header of the CSI data */
    uint8_t mac[6];                 /**< source MAC address of the CSI data */
    uint16_t raw_len;               /**< length of the CSI data */
    int8_t *raw_data;               /**< pointer to the CSI data，Unfiltered contains invalid subcarriers*/
    int16_t valid_len;              /**< length of the CSI data after filtering */
    uint8_t valid_llft_len;         /**< length of the LL-LTF data after filtering */
    uint8_t valid_ht_lft_len;       /**< length of the HT-LTF data after filtering */
    uint8_t valid_stbc_ht_lft_len;  /**< length of the STBC-HT-LTF data after filtering */
    int8_t valid_data[0];           /**< pointer to the CSI data after filtering */
} wifi_csi_filtered_info_t;


/**
  * @brief The RX callback function of Wi-Fi radar data.
  *
  *        Each time Wi-Fi radar data analysis, the callback function will be called.
  *
  * @param info Wi-Fi radar data received. The memory that it points to will be deallocated after callback function returns.
  * @param ctx context argument, passed to esp_radar_set_config() when registering callback function.
  *
  */
typedef void (*wifi_radar_cb_t)(const wifi_radar_info_t *info, void *ctx);

/**
 * @brief The RX callback function of Wi-Fi CSI data.
 * 
 */
typedef void (*wifi_csi_filtered_cb_t)(const wifi_csi_filtered_info_t *info, void *ctx);

/**
  * @brief Wi-Fi radar configuration type
  */
typedef struct {
    uint8_t filter_mac[6];                 /**< Get the mac of the specified device, no filtering: [0xff:0xff:0xff:0xff:0xff:0xff] */
    uint8_t filter_dmac[6];                /**< Get the destination mac of the specified device, no filtering: [0xff:0xff:0xff:0xff:0xff:0xff] */
    uint16_t filter_len;                   /**< source MAC address of the CSI data, no filtering: 0 */
    wifi_radar_cb_t wifi_radar_cb;         /**< Register the callback function of Wi-Fi radar data */
    void *wifi_radar_cb_ctx;               /**< Context argument, passed to callback function of Wi-Fi radar */
    wifi_csi_filtered_cb_t wifi_csi_filtered_cb; /**< Register the callback function of Wi-Fi CSI data */
    void *wifi_csi_cb_ctx;                 /**< Context argument, passed to callback function of Wi-Fi CSI */

    /**< Algorithm configuration */
    struct {
        UBaseType_t csi_handle_priority;  /**< The priority of the task that handles the CSI data */
        UBaseType_t csi_combine_priority; /**< The priority of the task that combines the CSI data */
        uint16_t csi_recv_interval;       /**< The interval of receiving CSI data, unit: ms */
        uint16_t csi_handle_time;         /**< The time of handling CSI data, unit: ms */
    };

    /**< Channel state information(CSI) configuration type */
    wifi_csi_config_t csi_config;
} wifi_radar_config_t;

/**
  * @brief Wi-Fi radar default configuration
  */
#define WIFI_RADAR_CONFIG_DEFAULT() { \
    .filter_mac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, \
    .filter_dmac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, \
    .wifi_radar_cb = NULL, \
    .wifi_radar_cb_ctx = NULL, \
    .wifi_csi_filtered_cb = NULL, \
    .wifi_csi_cb_ctx = NULL, \
    .csi_handle_priority = configMAX_PRIORITIES, \
    .csi_combine_priority = configMAX_PRIORITIES, \
    .csi_recv_interval    = 10, \
    .csi_handle_time      = 250, \
    .csi_config = { \
        .lltf_en           = true, \
        .htltf_en          = true, \
        .stbc_htltf2_en    = true, \
        .ltf_merge_en      = true, \
        .channel_filter_en = true, \
        .manu_scale        = true, \
        .shift             = true, \
    } \
}
  
/**
  * @brief Set Wi-Fi radar configuration
  *
  * @param config configuration
  *
  * return
  *    - ESP_OK
  *    - ESP_ERR_INVALID_ARG
  */
esp_err_t esp_radar_set_config(const wifi_radar_config_t *config);

/**
  * @brief Get Wi-Fi radar configuration
  *
  * @param config configuration
  *
  * return
  *    - ESP_OK
  *    - ESP_ERR_INVALID_ARG
  */
esp_err_t esp_radar_get_config(wifi_radar_config_t *config);

/**
  * @brief  Start WiFi radar according to current configuration
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_radar_start(void);

/**
  * @brief  Stop WiFi radar
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_radar_stop(void);

/**
  * @brief  Init WiFi radar
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_radar_init(void);

/**
  * @brief  Deinit Wi-Fi radar
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_radar_deinit(void);

/**
  * @brief  Start WiFi radar training
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_radar_train_start(void);

/**
  * @brief  Remove WiFi radar training
  *
  * @return
  *    - ESP_OK
  *    - ESP_FAIL
  */
esp_err_t esp_radar_train_remove(void);

/**
 * @brief It stops the calibration process and returns the wander and jitter thresholds
 *
 * @param wander_threshold The threshold for the wander state.
 * @param jitter_threshold The threshold of the correlation coefficient of the static state.
 *
 * @return The wander and jitter thresholds.
 */
esp_err_t esp_radar_train_stop(float *wander_threshold, float *jitter_threshold);

#ifdef __cplusplus
}
#endif
