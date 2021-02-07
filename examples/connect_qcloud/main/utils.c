// Copyright 2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "esp_event.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_log.h"

bool mac_str2hex(const char *mac_str, uint8_t *mac_hex)
{
    uint32_t mac_data[6] = {0};
    int ret = sscanf(mac_str, MACSTR, mac_data, mac_data + 1, mac_data + 2,
                     mac_data + 3, mac_data + 4, mac_data + 5);

    for (int i = 0; i < 6; i++) {
        mac_hex[i] = mac_data[i];
    }

    return ret == 6 ? true : false;
}

float avg(const float *a, size_t len)
{
    if (!len) {
        return 0.0;
    }

    float sum = 0;

    for (int i = 0; i < len; i++) {
        sum += a[i];
    }

    return sum / len;
}

static int cmp_float(const void *a, const void *b)
{
    return *(float *)a > *(float *)b ? 1 : -1;
}

float trimmean(const float *array, size_t len, float percent)
{
    float trimmean_data = 0;
    float *tmp = malloc(sizeof(float) * len);

    memcpy(tmp, array, sizeof(float) * len);
    qsort(tmp, len, sizeof(float), cmp_float);
    trimmean_data = avg(tmp + (int)(len * percent / 2 + 0.5), (int)(len * (1 - percent) + 0.5));

    free(tmp);

    return trimmean_data;
}

float max(const float *array, size_t len, float percent)
{
    float max_data = 0;
    float *tmp = malloc(sizeof(float) * len);

    memcpy(tmp, array, sizeof(float) * len);
    qsort(tmp, len, sizeof(float), cmp_float);
    max_data = tmp[len - (int)(len * percent + 0.5)];

    free(tmp);
    return max_data;
}
