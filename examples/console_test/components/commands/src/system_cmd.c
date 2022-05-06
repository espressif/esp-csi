// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp_log.h"

#include <sys/param.h>
#include "argtable3/argtable3.h"
#include "mbedtls/base64.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_partition.h"
#include "esp_console.h"

static const char *TAG = "system_cmd";

/**
 * @brief  A function which implements version command.
 */
static int version_func(int argc, char **argv)
{
    esp_chip_info_t chip_info = {0};

    /**< Pint system information */
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "compile time     : %s %s", __DATE__, __TIME__);
    ESP_LOGI(TAG, "free heap        : %d Bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "CPU cores        : %d", chip_info.cores);
    ESP_LOGI(TAG, "silicon revision : %d", chip_info.revision);
    ESP_LOGI(TAG, "feature          : %s%s%s%s%d%s",
             chip_info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
             chip_info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
             chip_info.features & CHIP_FEATURE_BT ? "/BT" : "",
             chip_info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
             spi_flash_get_chip_size() / (1024 * 1024), " MB");

    return ESP_OK;
}

/**
 * @brief  Register version command.
 */
static void register_version()
{
    const esp_console_cmd_t cmd = {
        .command = "version",
        .help = "Get version of chip and SDK",
        .hint = NULL,
        .func = &version_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}


/**
 * @brief  A function which implements restart command.
 */
static int restart_func(int argc, char **argv)
{
    ESP_LOGI(TAG, "Restarting");
    esp_restart();
}

/**
 * @brief  Register restart command.
 */
static void register_restart()
{
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help    = "Software reset of the chip",
        .hint    = NULL,
        .func    = &restart_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**
 * @brief  A function which implements reset command.
 */
static int reset_func(int argc, char **argv)
{
    esp_partition_iterator_t part_itra = NULL;
    const esp_partition_t *nvs_part = NULL;

    ESP_LOGI(TAG, "Erase part of the nvs partition");

    part_itra = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "nvs");
    nvs_part = esp_partition_get(part_itra);
    esp_partition_erase_range(nvs_part, 0, nvs_part->size);

    ESP_LOGI(TAG, "Restarting");
    esp_restart();
}

/**
 * @brief  Register reset command.
 */
static void register_reset()
{
    const esp_console_cmd_t cmd = {
        .command = "reset",
        .help = "Clear device configuration information",
        .hint = NULL,
        .func = &reset_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**
 * @brief  A function which implements heap command.
 */
static int heap_func(int argc, char **argv)
{
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
    TaskStatus_t *pxTaskStatusArray = NULL;
    volatile UBaseType_t uxArraySize = 0;
    uint32_t ulTotalRunTime = 0, ulStatsAsPercentage = 0, ulRunTimeCounte = 0;
    const char task_status_char[] = {'r', 'R', 'B', 'S', 'D'};

    /* Take a snapshot of the number of tasks in case it changes while this
    function is executing. */
    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = malloc(uxTaskGetNumberOfTasks() * sizeof(TaskStatus_t));

    if (!pxTaskStatusArray) {
        return ;
    }

    /* Generate the (binary) data. */
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
    ulTotalRunTime /= 100UL;

    ESP_LOGI(TAG, "---------------- The State Of Tasks ----------------");
    ESP_LOGI(TAG, "- HWM   : usage high water mark (Byte)");
    ESP_LOGI(TAG, "- Status: blocked ('B'), ready ('R'), deleted ('D') or suspended ('S')\n");
    ESP_LOGI(TAG, "TaskName\t\tStatus\tPrio\tHWM\tTaskNum\tCoreID\tRunTimeCounter\tPercentage");

    for (int i = 0; i < uxArraySize; i++) {
#if( configGENERATE_RUN_TIME_STATS == 1 )
        ulRunTimeCounte = pxTaskStatusArray[i].ulRunTimeCounter;
        ulStatsAsPercentage = ulRunTimeCounte / ulTotalRunTime;
#else
#warning configGENERATE_RUN_TIME_STATS must also be set to 1 in FreeRTOSConfig.h to use vTaskGetRunTimeStats().
#endif

        int core_id = -1;
        char precentage_char[4] = {0};

#if ( configTASKLIST_INCLUDE_COREID == 1 )
        core_id = (int) pxTaskStatusArray[ i ].xCoreID;
#else
#warning configTASKLIST_INCLUDE_COREID must also be set to 1 in FreeRTOSConfig.h to use xCoreID.
#endif

        /* Write the rest of the string. */
        ESP_LOGI(TAG, "%-16s\t%c\t%u\t%u\t%u\t%hd\t%-16u%-s%%",
                 pxTaskStatusArray[i].pcTaskName, task_status_char[pxTaskStatusArray[i].eCurrentState],
                 (uint32_t) pxTaskStatusArray[i].uxCurrentPriority,
                 (uint32_t) pxTaskStatusArray[i].usStackHighWaterMark,
                 (uint32_t) pxTaskStatusArray[i].xTaskNumber, core_id,
                 ulRunTimeCounte, (ulStatsAsPercentage <= 0) ? "<1" : itoa(ulStatsAsPercentage, precentage_char, 10));
    }

    free(pxTaskStatusArray);
#endif /**< ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 */

#ifndef CONFIG_SPIRAM_SUPPORT
    ESP_LOGI(TAG, "Free heap, current: %d, minimum: %d",
             esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
#else
    ESP_LOGI(TAG, "Free heap, internal current: %d, minimum: %d, total current: %d, minimum: %d",
             heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
             heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
             esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
#endif /**< CONFIG_SPIRAM_SUPPORT */

    return ESP_OK;
}

/**
 * @brief  Register heap command.
 */
static void register_heap()
{
    const esp_console_cmd_t cmd = {
        .command = "heap",
        .help = "Get the current size of free heap memory",
        .hint = NULL,
        .func = &heap_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}


void cmd_register_system(void)
{
    register_version();
    register_heap();
    register_restart();
    register_reset();
}