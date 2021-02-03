#!/usr/bin/env python
#
# 'idf.py' is a top-level config/build command line tool for ESP-IDF
#
# You don't have to use idf.py, you can use cmake directly
# (or use cmake in an IDE)
#
#
# Copyright 2021 Espressif Systems (Shanghai) PTE LTD
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# WARNING: we don't check for Python build-time dependencies until
# check_environment() function below. If possible, avoid importing
# any external libraries here - put in external script, or import in
# their specific function instead.

# -*- coding: utf-8 -*-

import serial
import sys
import time
import math
import csv
import numpy as np

import matplotlib
import matplotlib.pyplot as plt

from math import sqrt, atan2

# Median filter
def median_filter(src, k=50):
    edge = int((k - 1) / 2)
    dst = []
    if len(src) - 1 - edge <= edge:
        print("The parameter k is to large.")
        return None

    for i in range(len(src)):
        if i <= edge - 1:
            dst.append(src[i])
        else:
            # nm:neighbour matrix
            nm = src[i - edge: i + edge + 1]

            if src[i] == np.max(nm) or src[i] == np.min(nm):
                dst.append(np.median(nm))
            else:
                dst.append(src[i])
    return dst


# Calculate the amplitude of CSI
def get_amplitude(amplitude_list, data_list, data_len):
    data_imaginary_list, data_real_list = [], []
    try:
        for i in range(0, data_len, 2):
            data_imaginary_list.append(int(data_list[i]))
            data_real_list.append(int(data_list[i + 1]))
    except Exception as e:
        print(e)
        return None

    for i in range(data_len // 2):
        amplitude_list[i].append(
            sqrt(data_imaginary_list[i]**2+data_real_list[i]**2))
    return None

# Get CSI data from file
def get_data_file(file_name, data_len_filter=384, max_line=10240):
    csv_file = csv.reader(open(file_name, 'r'))
    amplitude_len = 0
    amplitude_list = [[]for i in range(data_len_filter//2)]
    rssi_list = []

    for row in csv_file:
        if len(row) < 25:
            continue

        if "CSI_DATA" not in row[0]:
            continue

        data_len = int(row[22])
        if data_len != data_len_filter:
            continue

        data_list = (row[24].split('[ ')[1]).split(' ')
        get_amplitude(amplitude_list, data_list, data_len)
        amplitude_len += 1

        rssi_list.append(int(row[4]))

        if(amplitude_len > max_line):
            break

    return amplitude_len, amplitude_list, rssi_list

if __name__ == "__main__":
    file_name = ['/home/it_zzc/project/Wi-Fi_CSI/sort_out/esp-qcloud/examples/led_light/components/esp_rader/test/csi_data.csv']

    matplotlib.use('TkAgg') 


    for i in range(len( file_name )):
        rssi_static_std = []
        amplitude_len, amplitude_list, rssi_list = get_data_file(file_name[i])
        print(rssi_list)

        for j in range(0, 20):
            # rssi_filter_list = median_filter(rssi_list[j * 200:( j + 1) * 200 ],k = 50)
            rssi_static_std.append(np.var(rssi_list))
            # rssi_static_std.append(np.var(rssi_list))

        plt.plot(rssi_list)
        plt.legend()
        # plt.savefig("csi_amplitude.jpg", transparent=True, dpi=2000)
        plt.show()