#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "app_uart.h"
#include "bsp_C5_dual_antenna.h"
#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     2000000
#define TXD_PIN            (BSP_CNT_4)
#define RXD_PIN            (BSP_CNT_3)
#define BUF_SIZE           4096
static const char *TAG = "UART";
QueueHandle_t uart_recv_queue;
static QueueHandle_t uart0_queue;
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(2 * BUF_SIZE);
    csi_data_t* csi_data;
    uint16_t table_row_last=0;
    for (;;) {
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            switch (event.type) {
            case UART_DATA:
                UBaseType_t queue_size = uxQueueMessagesWaiting(uart0_queue);
                uart_read_bytes(UART_PORT_NUM, dtmp, event.size, portMAX_DELAY);
                // ESP_LOGI(TAG, "[UART DATA]: %d %u", event.size,queue_size);
                    for (int i = 0; i <= (int)event.size - 32; i++) {
                        if (dtmp[i] == 0xAA && dtmp[i + 1] == 0x55 && dtmp[i + 30] == 0x55 && dtmp[i + 31] == 0xAA) {
                            xQueueSend(uart_recv_queue,(csi_data_t*)&dtmp[i], 0);
                            csi_data = (csi_data_t *)&dtmp[i];
                            ESP_LOGD(TAG,"%d,%lld,%.2f",csi_data->start[0],csi_data->time_delta,csi_data->cir[0]);
                            i = i + 24-1;
                        }
                    }         
                break;
            case UART_FIFO_OVF:
                ESP_LOGW(TAG, "hw fifo overflow");
                uart_flush_input(UART_PORT_NUM);
                xQueueReset(uart0_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "ring buffer full");
                uart_flush_input(UART_PORT_NUM);
                xQueueReset(uart0_queue);
                break;
            case UART_BREAK:
                ESP_LOGD(TAG, "uart rx break");
                break;
            case UART_PARITY_ERR:
                ESP_LOGD(TAG, "uart parity error");
                break;
            case UART_FRAME_ERR:
                ESP_LOGD(TAG, "uart frame error");
                break;
            default:
                ESP_LOGD(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void init_uart() {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0 , 20, &uart0_queue, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_LOGI("UART", "UART initialized");
    uart_recv_queue = xQueueCreate(20, sizeof(csi_data_t));
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 9, NULL); // 核心 1

}

int uart_send_data(const char *data, uint8_t len) {
    return uart_write_bytes(UART_PORT_NUM, data, len);
}

// void uart_receive_data() {
//     uint8_t data[BUF_SIZE];
//     int length = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
//     if (length > 0) {
//         data[length] = '\0';
//         ESP_LOGI("UART", "Received %d bytes: '%s'", length, (char *)data);
//     }
// }
