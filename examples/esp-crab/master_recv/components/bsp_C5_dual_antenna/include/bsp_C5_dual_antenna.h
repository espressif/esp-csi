/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP BSP: ESP-Magnescreen
 */

#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "iot_button.h"
#include "display.h"
#include "soc/uart_pins.h"
#define MASTER_CHIP 1
#define SLAVE_CHIP 0
#define Self_Transmit_and_Receive_Mode 1
#define Single_Transmit_and_Dual_Receive_Mode 0

#if CHIP_ID == MASTER_CHIP

/* Display */
#define BSP_LCD_DATA0         (GPIO_NUM_6)
#define BSP_LCD_PCLK          (GPIO_NUM_5)
#define BSP_LCD_CS            (GPIO_NUM_NC)
#define BSP_LCD_DC            (GPIO_NUM_4)
#define BSP_LCD_RST           (GPIO_NUM_7)
#define LCD_BACKLIGHT_CHANNEL LEDC_CHANNEL_1
#define BSP_LCD_BACKLIGHT     (GPIO_NUM_3)
#define BSP_LCD_TOUCH_INT     (GPIO_NUM_10)
#define BSP_I2C_SCL           (GPIO_NUM_8)
#define BSP_I2C_SDA           (GPIO_NUM_9)

/* Buttons */
#define BSP_BUTTON            (GPIO_NUM_1)

/* Beep */
#define BSP_BEEP_PIN          (GPIO_NUM_0)
#define PWM_CHANNEL           LEDC_CHANNEL_0
#define PWM_FREQ_HZ           2731
#define PWM_RESOLUTION        LEDC_TIMER_10_BIT

/* LED */
#define BSP_LED_GPIO          (GPIO_NUM_2)

/* connect */
#define BSP_RESET_SLAVE       (GPIO_NUM_23)
#define BSP_CNT_1             (GPIO_NUM_24)
#define BSP_CNT_2             (GPIO_NUM_25)
#define BSP_CNT_3             (GPIO_NUM_26)
#define BSP_CNT_4             (GPIO_NUM_27)
#define BSP_CNT_5             (GPIO_NUM_10)
#define BSP_CNT_6             (GPIO_NUM_11)

#else 

#define BSP_LED_GPIO          (GPIO_NUM_10)

#define PIN_NUM_MISO  GPIO_NUM_7
#define PIN_NUM_MOSI  GPIO_NUM_5
#define PIN_NUM_CLK   GPIO_NUM_6
#define PIN_NUM_CS    GPIO_NUM_2

#endif
/* Buttons */
typedef enum {
    BSP_BUTTON_POWER = 0,
    BSP_BUTTON_NUM
} bsp_button_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BSP display configuration structure
 *
 */
typedef struct {
    lvgl_port_cfg_t lvgl_port_cfg;  /*!< LVGL port configuration */
    uint32_t        buffer_size;    /*!< Size of the buffer for the screen in pixels */
    bool            double_buffer;  /*!< True, if should be allocated two buffers */
    struct {
        unsigned int buff_dma: 1;    /*!< Allocated LVGL buffer will be DMA capable */
        unsigned int buff_spiram: 1; /*!< Allocated LVGL buffer will be in PSRAM */
    } flags;
} bsp_display_cfg_t;
typedef struct {
    uint32_t red;      // 红色值
    uint32_t green;    // 绿色值
    uint32_t blue;     // 蓝色值
    bool led_power;    // LED 电源开关状态
} RGB_LED_Config;

/**************************************************************************************************
 *
 * I2C interface
 *
 * There are multiple devices connected to I2C peripheral:
 *  - Codec ES8311 (configuration only)
 *  - ADC ES7210 (configuration only)
 *  - Encryption chip ATECC608A (NOT populated on most boards)
 *  - LCD Touch controller
 *  - Inertial Measurement Unit ICM-42607-P
 *
 * After initialization of I2C, use BSP_I2C_NUM macro when creating I2C devices drivers ie.:
 * \code{.c}
 * icm42670_handle_t imu = icm42670_create(BSP_I2C_NUM, ICM42670_I2C_ADDRESS);
 * \endcode
 **************************************************************************************************/
#define BSP_I2C_NUM     CONFIG_BSP_I2C_NUM

/**
 * @brief Init I2C driver
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   I2C parameter error
 *      - ESP_FAIL              I2C driver installation error
 *
 */
esp_err_t bsp_i2c_init(void);

/**
 * @brief Deinit I2C driver and free its resources
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   I2C parameter error
 *
 */
esp_err_t bsp_i2c_deinit(void);

/**************************************************************************************************
 *
 * SPIFFS
 *
 * After mounting the SPIFFS, it can be accessed with stdio functions ie.:
 * \code{.c}
 * FILE* f = fopen(BSP_SPIFFS_MOUNT_POINT"/hello.txt", "w");
 * fprintf(f, "Hello World!\n");
 * fclose(f);
 * \endcode
 **************************************************************************************************/
#define BSP_SPIFFS_MOUNT_POINT      CONFIG_BSP_SPIFFS_MOUNT_POINT

/**
 * @brief Mount SPIFFS to virtual file system
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if esp_vfs_spiffs_register was already called
 *      - ESP_ERR_NO_MEM if memory can not be allocated
 *      - ESP_FAIL if partition can not be mounted
 *      - other error codes
 */
esp_err_t bsp_spiffs_mount(void);

/**
 * @brief Unmount SPIFFS from virtual file system
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_FOUND if the partition table does not contain SPIFFS partition with given label
 *      - ESP_ERR_INVALID_STATE if esp_vfs_spiffs_unregister was already called
 *      - ESP_ERR_NO_MEM if memory can not be allocated
 *      - ESP_FAIL if partition can not be mounted
 *      - other error codes
 */
esp_err_t bsp_spiffs_unmount(void);

/**************************************************************************************************
 *
 * LCD interface
 *
 * ESP-BOX is shipped with 2.4inch ST7789 display controller.
 * It features 16-bit colors, 320x240 resolution and capacitive touch controller.
 *
 * LVGL is used as graphics library. LVGL is NOT thread safe, therefore the user must take LVGL mutex
 * by calling bsp_display_lock() before calling and LVGL API (lv_...) and then give the mutex with
 * bsp_display_unlock().
 *
 * Display's backlight must be enabled explicitly by calling bsp_display_backlight_on()
 **************************************************************************************************/
#define BSP_LCD_PIXEL_CLOCK_HZ     (40 * 1000 * 1000)
#define BSP_LCD_SPI_NUM            (SPI2_HOST)

/**
 * @brief Initialize display
 *
 * This function initializes SPI, display controller and starts LVGL handling task.
 * LCD backlight must be enabled separately by calling bsp_display_brightness_set()
 *
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_disp_t *bsp_display_start(void);

/**
 * @brief Initialize display
 *
 * This function initializes SPI, display controller and starts LVGL handling task.
 * LCD backlight must be enabled separately by calling bsp_display_brightness_set()
 *
 * @param cfg display configuration
 *
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_disp_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg);
esp_err_t bsp_display_brightness_set(int brightness_percent);
/**
 * @brief Get pointer to input device (touch, buttons, ...)
 *
 * @note The LVGL input device is initialized in bsp_display_start() function.
 *
 * @return Pointer to LVGL input device or NULL when not initialized
 */
lv_indev_t *bsp_display_get_input_dev(void);

/**
 * @brief Take LVGL mutex
 *
 * @param timeout_ms Timeout in [ms]. 0 will block indefinitely.
 * @return true  Mutex was taken
 * @return false Mutex was NOT taken
 */
bool bsp_display_lock(uint32_t timeout_ms);

/**
 * @brief Give LVGL mutex
 *
 */
void bsp_display_unlock(void);

/**
 * @brief Set display enter sleep mode
 *
 * All the display (LCD, backlight, touch) will enter sleep mode.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
esp_err_t bsp_display_enter_sleep(void);

/**
 * @brief Set display exit sleep mode
 *
 * All the display (LCD, backlight, touch) will exit sleep mode.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
esp_err_t bsp_display_exit_sleep(void);

/**
 * @brief Rotate screen
 *
 * Display must be already initialized by calling bsp_display_start()
 *
 * @param[in] disp Pointer to LVGL display
 * @param[in] rotation Angle of the display rotation
 */
void bsp_display_rotate(lv_disp_t *disp, lv_disp_rot_t rotation);

esp_err_t bsp_pwm_init(void);
esp_err_t bsp_pwm_set_duty(uint8_t duty_percent);
esp_err_t bsp_led_init(void);
esp_err_t bsp_led_power_off(void);
esp_err_t bsp_led_set(uint32_t index, uint32_t red, uint32_t green, uint32_t blue);
esp_err_t bsp_led_deinit(void);
esp_err_t bsp_slave_reset(void);

#ifdef __cplusplus
}
#endif
