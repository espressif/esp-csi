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

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

bool mac_str2hex(const char *mac_str, uint8_t *mac_hex);
float avg(const float *array, size_t len);
float max(const float *array, size_t len, float percent);
float trimmean(const float *array, size_t len, float percent);

void cmd_register_ping(void);
void cmd_register_system(void);
void cmd_register_wifi(void);
void cmd_register_csi(void);
void cmd_register_detect(void);

#ifdef __cplusplus
}
#endif
