#include <string.h>
#include <stdio.h>
#include "app_ui.h"
#include "ui.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_C5_dual_antenna.h"
#include "app_uart.h"
#include <math.h>

#define DISPLAY_SAMPLE_STEP 3 
#define LVGL_CHART_POINTS   (100 / DISPLAY_SAMPLE_STEP)
#define PI 3.14159265
#define SAMPLE_RATE LVGL_CHART_POINTS

extern QueueHandle_t uart_recv_queue;
extern  QueueHandle_t csi_display_queue;
extern csi_data_t master_data[DATA_TABLE_SIZE];
lv_chart_series_t * ser[6];
int16_t sine_wave[LVGL_CHART_POINTS*3];
float angle = 0;
static const char *TAG = "app_ui";

void generate_sine_wave(int16_t *data) 
{
    int16_t num_samples = SAMPLE_RATE*3;
    for (int i = 0; i < num_samples; i++) {
        data[i] = (int16_t)(100 * sin(2 * PI * ((float)i / (float)SAMPLE_RATE)));
    }
}

int get_sine_wave_index(float angle) 
{
    int num_samples = SAMPLE_RATE * 3;
    int index = (int)((angle / (2 * PI)) * SAMPLE_RATE);
    if (index <= SAMPLE_RATE/2) {
        index += SAMPLE_RATE;  
    }
    return (index-SAMPLE_RATE/2);
}

void ScreenSSliderLight_function(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    int slider_value = lv_slider_get_value(target);
    bsp_display_brightness_set(slider_value);
} 

void PhaseCalibration_button(lv_event_t * e)
{
    angle =0;
    ESP_LOGI(TAG, "PhaseCalibration_button");
}


void app_ui_init(void) 
{   
    lv_chart_set_range(ui_ScreenW_Chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(ui_ScreenW_Chart, LVGL_CHART_POINTS);
    ser[0] = lv_chart_add_series(ui_ScreenW_Chart, lv_color_hex(0x388E3C), LV_CHART_AXIS_PRIMARY_Y); // 深绿
    ser[1] = lv_chart_add_series(ui_ScreenW_Chart, lv_color_hex(0x81C784), LV_CHART_AXIS_PRIMARY_Y); // 浅绿
    ser[4] = lv_chart_add_series(ui_ScreenW_Chart, lv_color_hex(0x1976D2), LV_CHART_AXIS_PRIMARY_Y); // 深蓝
    ser[5] = lv_chart_add_series(ui_ScreenW_Chart, lv_color_hex(0x64B5F6), LV_CHART_AXIS_PRIMARY_Y); // 浅蓝
    lv_chart_set_range(ui_ScreenWP_Chart, LV_CHART_AXIS_PRIMARY_Y, -100, 100);
    lv_chart_set_point_count(ui_ScreenWP_Chart, LVGL_CHART_POINTS);
    ser[2] = lv_chart_add_series(ui_ScreenWP_Chart, lv_color_hex(0x1976D2), LV_CHART_AXIS_PRIMARY_Y); // 深蓝
    ser[3] = lv_chart_add_series(ui_ScreenWP_Chart, lv_color_hex(0x64B5F6), LV_CHART_AXIS_PRIMARY_Y); // 浅蓝

    lv_chart_refresh(ui_ScreenW_Chart);
    lv_chart_refresh(ui_ScreenWP_Chart);
    generate_sine_wave(sine_wave);
}

float circular_difference(float angle1, float angle2) 
{
    float diff = fmod(angle2 - angle1 + PI, 2 * PI);
    if (diff < 0) {
        diff += 2 * PI;
    }
    return diff - PI;
}

void csi_data_display_task(void *arg)         
{
    app_ui_init();
    static uint16_t y_range[2] ={0};
    uint8_t count=0;
    csi_data_t csi_display_data;

    uint8_t sine_offest[2];
    uint8_t cnt=0;
    float range[4][LVGL_CHART_POINTS] = {};
    uint8_t csi_mode = *((bool *)arg);
    if (csi_mode){
        ESP_LOGI(TAG,"Self_Transmit_and_Receive_Mode");
    } else {
        ESP_LOGI(TAG,"Single_Transmit_and_Dual_Receive_Mode");
    }
    if (csi_mode){
        while (xQueueReceive(csi_display_queue, &csi_display_data, portMAX_DELAY) == pdTRUE) {
            cnt++;
            UBaseType_t queueLength = uxQueueMessagesWaiting(csi_display_queue);
            if (queueLength>10){
                ESP_LOGI(TAG, "ui queueLength:%d", queueLength);
            }           
            range[0][count] = csi_display_data.cir[0]*5;
            range[1][count] = csi_display_data.cir[1]*5;
            range[2][count] = csi_display_data.cir[2];
            range[3][count] = csi_display_data.cir[3];

            y_range[0] = 500;
            y_range[1] = 0;
            for (int i=0;i<LVGL_CHART_POINTS;i++){
                if (y_range[0]>range[0][i]){
                    y_range[0] = range[0][i];
                }
                if (y_range[0]>range[1][i]){
                    y_range[0] = range[1][i];
                }
                if (y_range[1]<range[0][i]){
                    y_range[1] = range[0][i];
                }
                if (y_range[1]<range[1][i]){
                    y_range[1] = range[1][i];
                }
            }
            if  ( (y_range[1]-y_range[0])<100){
                y_range[1] += (100-y_range[1]+y_range[0])/2;
                y_range[0] -= (100-y_range[1]+y_range[0])/2;
            }
            lvgl_port_lock(0);
            lv_chart_set_next_value(ui_ScreenW_Chart, ser[0], (uint16_t)(range[0][count]));
            lv_chart_set_next_value(ui_ScreenW_Chart, ser[1], (uint16_t)(range[1][count]));

            float sum[2]={0};
            for (int i=count;i>count-20;i--){
                uint8_t index = (i+LVGL_CHART_POINTS)%LVGL_CHART_POINTS;
                sum[0] += circular_difference(range[2][count], range[2][index]);
                sum[1] += circular_difference(range[3][count], range[3][index]);
            }
            sum[0] = fmod(sum[0]/20 + range[2][count] + 2 * PI, 2 * PI) - PI;
            sum[1] = fmod(sum[1]/20 + range[3][count] + 2 * PI, 2 * PI) - PI;

            sine_offest[0] = get_sine_wave_index(sum[0]);
            sine_offest[1] = get_sine_wave_index(sum[1]);
            lv_chart_set_ext_y_array(ui_ScreenWP_Chart, ser[2], sine_wave+sine_offest[0]);
            lv_chart_set_range(ui_ScreenW_Chart, LV_CHART_AXIS_PRIMARY_Y, y_range[0], y_range[1]);

            lvgl_port_unlock();

            count++;
            if (count == LVGL_CHART_POINTS) {
                count = 0;
            }
        }
    }else {
        static csi_data_t csi_display_data_new = {0};
        while (xQueueReceive(uart_recv_queue, &csi_display_data_new, portMAX_DELAY) == pdTRUE) {
            cnt++;
            if (csi_display_data.start[0] == 0){
                goto NEXT;
            }
            
            csi_data_t csi_master_data  = master_data[csi_display_data.id % DATA_TABLE_SIZE];
            if (csi_master_data.id != csi_display_data.id){
                ESP_LOGE(TAG, "%ld id_master  %ld csi_display_data.id %ld",csi_display_data.id % DATA_TABLE_SIZE,csi_master_data.id,csi_display_data.id);
                goto NEXT;
            }
            range[0][count] = csi_display_data.cir[0]*5;
            range[1][count] = csi_master_data.cir[1]*5;
            range[2][count] = csi_master_data.cir[2] - csi_display_data.cir[2];
            range[3][count] = csi_display_data.cir[3];

            y_range[0] = 500;
            y_range[1] = 0;
            for (int i=0;i<LVGL_CHART_POINTS;i++){
                if (y_range[0]>range[0][i]){
                    y_range[0] = range[0][i];
                }
                if (y_range[0]>range[1][i]){
                    y_range[0] = range[1][i];
                }
                if (y_range[1]<range[0][i]){
                    y_range[1] = range[0][i];
                }
                if (y_range[1]<range[1][i]){
                    y_range[1] = range[1][i];
                }
            }
            if  ( (y_range[1]-y_range[0])<100){
                y_range[1] += (100-y_range[1]+y_range[0])/2;
                y_range[0] -= (100-y_range[1]+y_range[0])/2;
            }
            lvgl_port_lock(0);
            lv_chart_set_next_value(ui_ScreenW_Chart, ser[0], (uint16_t)(range[0][count]));
            lv_chart_set_next_value(ui_ScreenW_Chart, ser[1], (uint16_t)(range[1][count]));

            float sum[2]={0};
            for (int i=count;i>count-20;i--){
                uint8_t index = (i+LVGL_CHART_POINTS)%LVGL_CHART_POINTS;
                sum[0] += circular_difference(range[2][count], range[2][index]);
                sum[1] += circular_difference(range[3][count], range[3][index]);
            }
            sum[0] = fmod(sum[0]/20 + range[2][count] + 2 * PI, 2 * PI) - PI;
            sum[1] = fmod(sum[1]/20 + range[3][count] + 2 * PI, 2 * PI) - PI;
            
            sine_offest[0] = get_sine_wave_index(sum[0]);
            sine_offest[1] = get_sine_wave_index(sum[1]);
            lv_chart_set_ext_y_array(ui_ScreenWP_Chart, ser[2], sine_wave+sine_offest[0]);
            lv_chart_set_range(ui_ScreenW_Chart, LV_CHART_AXIS_PRIMARY_Y, y_range[0], y_range[1]);

            lvgl_port_unlock();

            count++;
            if (count == LVGL_CHART_POINTS) {
                count = 0;
            }
NEXT:
            csi_display_data = csi_display_data_new;
        }
    }
}