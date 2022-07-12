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
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_console.h"

#include "esp_wifi.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "mbedtls/base64.h"
#include "esp_radar.h"

#define RX_BUFFER_SIZE              1460
#define KEEPALIVE_IDLE              1
#define KEEPALIVE_INTERVAL          1
#define KEEPALIVE_COUNT             3
typedef union {
    struct  {
        const char *type;
        const char *id;
        const char *timestamp;
        const char *action_id;
        const char *action;
        const char *mac;
        const char *rssi;
        const char *rate;
        const char *sig_mode;
        const char *mcs;
        const char *bandwidth;
        const char *smoothing;
        const char *not_sounding;
        const char *aggregation;
        const char *stbc;
        const char *fec_coding;
        const char *sgi;
        const char *noise_floor;
        const char *ampdu_cnt;
        const char *channel;
        const char *secondary_channel;
        const char *local_timestamp;
        const char *ant;
        const char *sig_len;
        const char *rx_state;
        const char *len;
        const char *first_word;
        const char *data;
    };
    const char *data_array[28];
} csi_data_str_t;

static char *TAG = "radar_evaluate";
static char g_wifi_radar_cb_ctx[32] = {0};
static TaskHandle_t g_tcp_server_task_handle = NULL;


static void csi_info_analysis(char *data, size_t size)
{
    uint8_t column = 0;
    const char *s = ",";
    char *token   = NULL;
    csi_data_str_t *csi_data = malloc(sizeof(csi_data_str_t));

    token = strtok(data, s);

    do {
        csi_data->data_array[column++] = token;
        token = strtok(NULL, s);
    } while (token != NULL && column < 28);

    if (token != NULL) {
        ESP_LOGE(TAG, "data error, temp_data: %s", data);
    }

    wifi_csi_filtered_info_t *filtered_info = malloc(sizeof(wifi_csi_filtered_info_t) + 52 * 2);
    mbedtls_base64_decode((uint8_t *)filtered_info->valid_data, 52 * 2, (size_t *)&filtered_info->valid_len, (uint8_t *)csi_data->data, strlen(csi_data->data));
    filtered_info->rx_ctrl.timestamp = atoi(csi_data->local_timestamp);
    strcpy(g_wifi_radar_cb_ctx, csi_data->timestamp);
    // ESP_LOGI(TAG,"count: %s, timestamp: %d", csi_data->id, filtered_info->rx_ctrl.timestamp);

    extern esp_err_t csi_data_push(wifi_csi_filtered_info_t *info);
    csi_data_push(filtered_info);

    free(csi_data);
}

static void tcp_server_task(void *arg)
{
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;
    uint32_t port = (uint32_t)arg;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(port);
        ip_protocol = IPPROTO_IP;
    }

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);

    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d, errno_str: %s", errno, strerror(errno));
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }

    ESP_LOGI(TAG, "Socket bound, port %d", port);

    err = listen(listen_sock, 1);

    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {
        ESP_LOGI(TAG, "Socket listening");
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);

        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }

        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        int len;
        char *rx_buffer = malloc(RX_BUFFER_SIZE);
        size_t buf_size = 0;

        wifi_radar_config_t radar_config = {0};
        esp_radar_get_config(&radar_config);
        radar_config.wifi_radar_cb_ctx = g_wifi_radar_cb_ctx;
        esp_radar_set_config(&radar_config);

        do {
            len = recv(sock, rx_buffer + buf_size, RX_BUFFER_SIZE - buf_size - 1, 0);

            // ESP_LOGW(TAG, "len: %d, rx_buffer: %s", len, rx_buffer);
            if (len < 0) {
                ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);

            } else if (len == 0) {
                ESP_LOGW(TAG, "Connection closed");
            } else {
                buf_size += len;
                rx_buffer[buf_size] = 0; // Null-terminate whatever is received and treat it like a string
                // ESP_LOGI(TAG, "Received %d, buf_size: %d", len, buf_size);

                while (buf_size > 250) {
                    char *begin = strstr(rx_buffer, "CSI_DATA");
                    char *end   = strstr(rx_buffer, "\n");
                    ssize_t size  = end - begin;

                    // ESP_LOGW("TAG", "begin: %p, end: %p, size: %d", begin, end, size);
                    // ESP_LOGW(TAG, "size: %d, received: %d, buf_size: %d", size, len, buf_size);

                    if (begin && end && size > 250) {
                        begin[size] = '\0';
                        csi_info_analysis(begin, size);
                    }

                    if (end) {
                        buf_size -= (end + 1 - rx_buffer);
                        memcpy(rx_buffer, end + 1, buf_size);
                        memset(rx_buffer + buf_size, 0, end + 1 - rx_buffer);
                    }

                    if (begin && !end) {
                        break;
                    }
                }
            }
        } while (len > 0);

        shutdown(sock, 0);
        close(sock);
        free(rx_buffer);

        ESP_LOGW(TAG, "Socket close: %s", addr_str);

        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_radar_get_config(&radar_config);
        radar_config.wifi_radar_cb_ctx = NULL;
        esp_radar_set_config(&radar_config);
    }

CLEAN_UP:
    close(listen_sock);
    g_tcp_server_task_handle = NULL;
    vTaskDelete(NULL);
}


esp_err_t rader_evaluate_server(uint32_t port)
{
    if (!g_tcp_server_task_handle) {
        xTaskCreate(tcp_server_task, "tcp_server", 4 * 1024, (void *)port, 0, &g_tcp_server_task_handle);
    }

    return ESP_OK;
}