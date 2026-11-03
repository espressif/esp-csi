/*
 * SPDX-FileCopyrightText: 2026 vb3227
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "screen_text.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern lv_obj_t *_canvas;
extern SemaphoreHandle_t xLvglMutex;

void drawScreenText(const String &text, int16_t x, int16_t y, uint32_t color)
{
    if (!_canvas || !xLvglMutex || !text.length()) {
        return;
    }
    xSemaphoreTake(xLvglMutex, portMAX_DELAY);
    const int width = lv_obj_get_width(_canvas);
    const int height = lv_obj_get_height(_canvas);
    if (x < width && y < height) {
        lv_draw_label_dsc_t label;
        lv_draw_label_dsc_init(&label);
        label.font = LV_FONT_DEFAULT;
        label.color = lv_color_hex(color);
        label.opa = LV_OPA_COVER;
        lv_canvas_draw_text(_canvas, x, y, width - max(0, static_cast<int>(x)), &label, text.c_str());
    }
    xSemaphoreGive(xLvglMutex);
}
