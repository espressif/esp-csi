#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <sys/types.h>

#define GPIO_INPUT_IO        27
#define GPIO_INPUT_PIN_SEL  (1ULL << GPIO_INPUT_IO)
extern int64_t time_zero;
static const char *TAG = "GPIO";
extern QueueHandle_t csi_recv_queue;

static void IRAM_ATTR gpio_isr_handler(void *arg) 
{
    xQueueReset(csi_recv_queue);
    time_zero = esp_timer_get_time();
    uint32_t gpio_num = (uint32_t)arg;
}

void init_gpio() 
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_INPUT_IO, gpio_isr_handler, (void *)GPIO_INPUT_IO);
    ESP_LOGI(TAG, "GPIO %d configured with negative edge interrupt.", GPIO_INPUT_IO);
}