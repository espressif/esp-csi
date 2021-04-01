/* LED Light Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_qcloud_log.h"
#include "esp_qcloud_console.h"
#include "esp_qcloud_storage.h"
#include "esp_qcloud_iothub.h"
#include "esp_qcloud_prov.h"

#include "light_driver.h"


typedef struct {
    struct { /**< status */
        size_t awake_count;
        float awake_amplitude_corr;
        float awake_amplitude_std;
    };

    struct { /**< config */
        bool threshold_adjust;
        float move_relative_threshold;
        float move_absolute_threshold;
        float threshold_human_detect;
    };

    struct { /**< result */
        bool room_status;
        bool human_move;
    };

    bool status_enable;
} wifi_radar_data_t;

esp_err_t qcloud_light_init(void);

void qcloud_light_report_status(void *avg);

esp_err_t wifi_radar_get_data(wifi_radar_data_t *data);
esp_err_t wifi_radar_set_data(const wifi_radar_data_t *data);
