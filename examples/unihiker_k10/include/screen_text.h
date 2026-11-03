/*
 * SPDX-FileCopyrightText: 2026 vb3227
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <Arduino.h>
#include <lvgl.h>

void drawScreenText(const String &text, int16_t x = 8, int16_t y = 8, uint32_t color = 0xFFFFFF);
