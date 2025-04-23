/* Esp-crab Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "nvs_flash.h"
#include "esp_mac.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "app_ifft.h"
#include "app_gpio.h"
#include "esp_timer.h"
#include "app_uart.h"
#include "IQmathLib.h"
#include "bsp_C5_dual_antenna.h"
#include "ui.h"
#include "app_ui.h"
#include "esp_radar.h"

#define CONFIG_LESS_INTERFERENCE_CHANNEL    40
#define CONFIG_WIFI_BAND_MODE               WIFI_BAND_MODE_5G_ONLY
#define CONFIG_WIFI_2G_BANDWIDTHS           WIFI_BW_HT40
#define CONFIG_WIFI_5G_BANDWIDTHS           WIFI_BW_HT40
#define CONFIG_WIFI_2G_PROTOCOL             WIFI_PROTOCOL_11N
#define CONFIG_WIFI_5G_PROTOCOL             WIFI_PROTOCOL_11N
#define CONFIG_ESP_NOW_PHYMODE              WIFI_PHY_MODE_HT40
#define CONFIG_GAIN_CONTROL                 1   // 1:enable gain control, 0:disable gain control
#define CONFIG_FORCE_GAIN                   0   // 1:force gain control, 0:automatic gain control
#define CONFIG_PRINT_CSI_DATA               0
#define CONFIG_CRAB_MODE                    Single_Transmit_and_Dual_Receive_Mode

csi_data_t master_data[DATA_TABLE_SIZE];
int64_t time_zero=0;
typedef struct {
    uint32_t id;
    uint32_t time;
    int8_t fft_gain;
    uint8_t agc_gain;
    int8_t buf[256];
} csi_recv_queue_t;
uint32_t recv_cnt = 0;
QueueHandle_t csi_recv_queue;
QueueHandle_t csi_display_queue;
static const uint8_t CONFIG_CSI_SEND_MAC[] = {0x1a, 0x00, 0x00, 0x00, 0x00, 0x00};
static const char *TAG = "csi_recv";

static void wifi_init()
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    esp_wifi_set_band_mode(CONFIG_WIFI_BAND_MODE);
    wifi_protocols_t protocols = {
        .ghz_2g = CONFIG_WIFI_2G_PROTOCOL,
        .ghz_5g = CONFIG_WIFI_5G_PROTOCOL
    };
    ESP_ERROR_CHECK(esp_wifi_set_protocols(ESP_IF_WIFI_STA, &protocols));

    wifi_bandwidths_t bandwidth = {
        .ghz_2g = CONFIG_WIFI_2G_BANDWIDTHS,
        .ghz_5g = CONFIG_WIFI_5G_BANDWIDTHS
    };
    ESP_ERROR_CHECK(esp_wifi_set_bandwidths(ESP_IF_WIFI_STA, &bandwidth));
 
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    
    if ((CONFIG_WIFI_BAND_MODE == WIFI_BAND_MODE_2G_ONLY && CONFIG_WIFI_2G_BANDWIDTHS == WIFI_BW_HT20) || (CONFIG_WIFI_BAND_MODE == WIFI_BAND_MODE_5G_ONLY && CONFIG_WIFI_5G_BANDWIDTHS == WIFI_BW_HT20))
        ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_LESS_INTERFERENCE_CHANNEL, WIFI_SECOND_CHAN_NONE));
    else
        ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_LESS_INTERFERENCE_CHANNEL, WIFI_SECOND_CHAN_BELOW));

    ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_STA, CONFIG_CSI_SEND_MAC));
}

static void wifi_esp_now_init(esp_now_peer_info_t peer) 
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)"pmk1234567890123"));
    esp_now_rate_config_t rate_config = {
        .phymode = CONFIG_ESP_NOW_PHYMODE, 
        .rate = WIFI_PHY_RATE_MCS0_LGI,    
        .ersu = false,                     
        .dcm = false                       
    };
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    ESP_ERROR_CHECK(esp_now_set_peer_rate_config(peer.peer_addr,&rate_config));
}

static void wifi_csi_rx_cb(void *ctx, wifi_csi_info_t *info)
{
    static int64_t last_time = 0;
    if (!info || !info->buf) {
        ESP_LOGW(TAG, "<%s> wifi_csi_cb", esp_err_to_name(ESP_ERR_INVALID_ARG));
        return;
    }

    if (memcmp(info->mac, CONFIG_CSI_SEND_MAC, 6)) {
        return;
    }

    const wifi_pkt_rx_ctrl_t *rx_ctrl = &info->rx_ctrl;
    static int s_count = 0;
    static uint8_t agc_gain=0; 
    static int8_t fft_gain=0; 
#if CONFIG_GAIN_CONTROL
    static uint8_t agc_gain_baseline=0; 
    static int8_t fft_gain_baseline=0; 
    esp_radar_get_rx_gain(info,&agc_gain,&fft_gain);
    if (s_count<100) {
        esp_radar_record_rx_gain(agc_gain, fft_gain);
    }else if (s_count == 100) {
        esp_radar_get_rx_gain_baseline(&agc_gain_baseline, &fft_gain_baseline);
        #if CONFIG_FORCE_GAIN 
        esp_radar_set_rx_force_gain(agc_gain_baseline, fft_gain_baseline);
        ESP_LOGI(TAG,"fft_force %d, agc_force %d",fft_gain_baseline,agc_gain_baseline);
        #endif
    }
#endif

    csi_recv_queue_t *csi_send_queuedata = (csi_recv_queue_t *)calloc(1, sizeof(csi_recv_queue_t));
    memcpy(&(csi_send_queuedata->id), info->payload+15, sizeof(uint32_t));

    csi_send_queuedata->time = info->rx_ctrl.timestamp;
    csi_send_queuedata->agc_gain = agc_gain;
    csi_send_queuedata->fft_gain = fft_gain;

    memset(csi_send_queuedata->buf, 0, 256);
    memcpy(csi_send_queuedata->buf + 8, info->buf, info->len);
    xQueueSend(csi_recv_queue, &csi_send_queuedata, 0);

#if CONFIG_PRINT_CSI_DATA
    if (!s_count) {
        ESP_LOGI(TAG, "================ CSI RECV ================");
        ets_printf("type,id,mac,rssi,rate,noise_floor,fft_gain,agc_gain,channel,local_timestamp,sig_len,rx_state,len,first_word,data\n");
    }

    ets_printf("CSI_DATA,%d," MACSTR ",%d,%d,%d,%d,%d,%d,%d,%d,%d",
            csi_send_queuedata->id, MAC2STR(info->mac), rx_ctrl->rssi, rx_ctrl->rate,
            rx_ctrl->noise_floor, fft_gain, agc_gain,rx_ctrl->channel,
            rx_ctrl->timestamp, rx_ctrl->sig_len, rx_ctrl->rx_state);

    ets_printf(",%d,%d,\"[%d", info->len, info->first_word_invalid, info->buf[0]);

    for (int i = 1; i < info->len; i++) {
        ets_printf(",%d", info->buf[i]);
    }

    ets_printf("]\"\n");
#endif
    s_count++;
    recv_cnt++;
}

static void wifi_csi_init()
{
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    csi_recv_queue = xQueueCreate(20, sizeof(csi_recv_queue_t *));
    csi_display_queue = xQueueCreate(20, sizeof(csi_data_t));
    wifi_csi_config_t csi_config = {
        .enable                   = true,                           
        .acquire_csi_legacy       = false,               
        .acquire_csi_force_lltf   = false,           
        .acquire_csi_ht20         = true,                  
        .acquire_csi_ht40         = true,                  
        .acquire_csi_vht          = false,                  
        .acquire_csi_su           = false,                   
        .acquire_csi_mu           = false,                   
        .acquire_csi_dcm          = false,                  
        .acquire_csi_beamformed   = false,           
        .acquire_csi_he_stbc_mode = 2,                                                                                                                                                                                                                                                                               
        .val_scale_cfg            = false,                    
        .dump_ack_en              = false,                      
        .reserved                 = false                         
    };

    ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));
    ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(wifi_csi_rx_cb, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_csi(true));
}

static void process_csi_data_task(void *pvParameter)
{   
    static int s_count = 0;
    csi_recv_queue_t *csi_recv_queue_data = NULL;
    Complex_Iq x_iq[64];
    float cir[2]={};
    float pha[2]={};
    while (xQueueReceive(csi_recv_queue, &csi_recv_queue_data, portMAX_DELAY) == pdTRUE) {
        UBaseType_t queueLength = uxQueueMessagesWaiting(csi_recv_queue);
        if (queueLength>10){
            ESP_LOGW(TAG, "csi queueLength:%d", queueLength);
        }     
#if !CONFIG_FORCE_GAIN && CONFIG_GAIN_CONTROL
        float scaling_factor =0;
        esp_radar_get_gain_compensation(&scaling_factor, csi_recv_queue_data->agc_gain, csi_recv_queue_data->fft_gain);
#else
        float scaling_factor = 1.0f;
#endif

        for (int i = 0; i < 64; i++) {
            x_iq[i].real = _IQ16(csi_recv_queue_data->buf[2 * i]); 
            x_iq[i].imag = _IQ16(csi_recv_queue_data->buf[2 * i + 1]);
        }
        fft_iq(x_iq, 1);
        cir[0] = complex_magnitude_iq(x_iq[0])* scaling_factor;
        pha[0] = complex_phase_iq(x_iq[0]);
        for (int i = 64; i < 128; i++) {
            x_iq[i-64].real = _IQ16(csi_recv_queue_data->buf[2 * i]); 
            x_iq[i-64].imag = _IQ16(csi_recv_queue_data->buf[2 * i + 1]);
        }
        fft_iq(x_iq, 1);
        cir[1] = complex_magnitude_iq(x_iq[0])* scaling_factor;
        pha[1] = complex_phase_iq(x_iq[0]);

        csi_data_t data = {
            .start = {0xAA, 0x55},
            .id = csi_recv_queue_data->id,
            .time_delta = csi_recv_queue_data->time-time_zero,
            .cir = {cir[0],cir[1],pha[0],pha[1]},
            .end = {0x55, 0xAA},
        };
        master_data[csi_recv_queue_data->id % DATA_TABLE_SIZE] = data;
        xQueueSend(csi_display_queue, &data, 0);
        free(csi_recv_queue_data);
    }
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    bsp_slave_reset();
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer = 0,
        .flags = {
            .buff_dma = true,
        }
    };
    bsp_display_start_with_config(&cfg);
    bsp_display_lock(0);
    ui_init();
    bsp_display_unlock();
    bsp_display_backlight_on();
    wifi_init();
    /**
     * @breif Initialize ESP-NOW
     *  ESP-NOW protocol see: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
     */
    esp_now_peer_info_t peer = {
        .channel   = CONFIG_LESS_INTERFERENCE_CHANNEL,
        .ifidx     = WIFI_IF_STA,    
        .encrypt   = false,   
        .peer_addr = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    };
#if CONFIG_IDF_TARGET_ESP32C5 || CONFIG_IDF_TARGET_ESP32C6
    wifi_esp_now_init(peer);
#endif
    init_gpio();

    wifi_csi_init();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    init_uart();
    bsp_led_init();

    xTaskCreate(process_csi_data_task, "process_csi_data_task", 4096, NULL, 6, NULL);
    bool arg = CONFIG_CRAB_MODE;
    xTaskCreate(csi_data_display_task, "csi_data_display_task", 4096, &arg, 6, NULL);
    uint32_t recv_cnt_prv=0;
    while(1) {
        static bool level = 1;
        static uint8_t time = 100;
        if ((recv_cnt -  recv_cnt_prv) >= 20){
            recv_cnt_prv = recv_cnt;
            time = 100;
            bsp_led_set(0, level*30,level*30, level*30);
            level = !level;
        }
        if (time>0){
            time-=1;
        }else{
            bsp_led_set(0,0,0,0);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
