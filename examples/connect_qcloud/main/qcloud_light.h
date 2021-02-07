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
} wifi_rader_data_t;

esp_err_t qcloud_light_init(void);

void qcloud_light_report_status(void *avg);

esp_err_t wifi_rader_get_data(wifi_rader_data_t *data);
esp_err_t wifi_rader_set_data(const wifi_rader_data_t *data);
