#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <sys/types.h>


#define GPIO_INPUT_IO     27 // 设置要使用的 GPIO 引脚编号
#define GPIO_INPUT_PIN_SEL  (1ULL << GPIO_INPUT_IO) // GPIO 位掩码
extern int64_t time_zero;
static const char *TAG = "GPIO";
extern QueueHandle_t csi_send_queue;
// 中断服务回调函数
static void IRAM_ATTR gpio_isr_handler(void *arg) {
    xQueueReset(csi_send_queue);
    time_zero = esp_timer_get_time(); 
    uint32_t gpio_num = (uint32_t)arg; // 获取中断的 GPIO 引脚编号
    // ets_printf("GPIO %ld triggered! time: %lld", gpio_num,time_since_boot);
}

// GPIO 配置和中断初始化
void init_gpio() {
    // 配置 GPIO 
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,      // 设置为下降沿中断
        .mode = GPIO_MODE_INPUT,            // 设置为输入模式
        .pin_bit_mask = GPIO_INPUT_PIN_SEL, // 设置 GPIO 位掩码
        .pull_up_en = GPIO_PULLUP_ENABLE,   // 启用上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 禁用下拉
    };
    gpio_config(&io_conf);

    // 安装 GPIO 中断服务
    gpio_install_isr_service(0);

    // 添加中断处理程序
    gpio_isr_handler_add(GPIO_INPUT_IO, gpio_isr_handler, (void *)GPIO_INPUT_IO);

    ESP_LOGI(TAG, "GPIO %d configured with negative edge interrupt.", GPIO_INPUT_IO);
}

