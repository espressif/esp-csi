#!/usr/bin/env python3
# -*-coding:utf-8-*-
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

import sys
from os import path
from io import StringIO
import argparse
import csv
import json


COLUMN = [
    "type",
    "id",
    "mac",
    "rssi",
    "rate",
    "sig_mode",
    "mcs",
    "bandwidth",
    "smoothing",
    "not_sounding",
    "aggregation",
    "stbc",
    "fec_coding",
    "sgi",
    "noise_floor",
    "ampdu_cnt",
    "channel",
    "secondary_channel",
    "local_timestamp",
    "ant",
    "sig_len",
    "rx_state",
    "len",
    "first_word",
    "data"]


def extract_csi_data(strings: str, num: int):
    """ extract csi data from string beginning with "CSI_DATA"

    :strings: string beginning with "CSI_DATA"
    :num: the elements number in strings
    :returns: List[type,id,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,len,first_word,data]

    """
    f = StringIO(strings)
    csv_reader = csv.reader(f)
    csi_data = next(csv_reader)

    if len(csi_data) != num:
        raise ValueError(f"element number is not equal {num}")

    try:
        json.loads(csi_data[-1])
    except json.JSONDecodeError:
        raise ValueError(f"data is not incomplete")
        
    return csi_data


if __name__ == '__main__':
    if sys.version_info < (3, 6):
        print(" Python version should >= 3.6")
        exit()

    parser = argparse.ArgumentParser(
        description="filter CSI_DATA from console_test logfile")
    parser.add_argument('-S', '--src', dest='src_file', action='store', required=True,
                        help="console_test logfile")
    parser.add_argument('-D', '--dst', dest='dst_file', action='store', default=None,
                        help="output file saved csi data")
    args = parser.parse_args()

    src_file = args.src_file
    dst_file = args.dst_file
    if dst_file is None:
        dst_file = path.splitext(path.basename(src_file))[0] + ".csv"

    f_src = open(src_file, 'r')
    f_dst = open(dst_file, 'w')
    csv_writer = csv.writer(f_dst)
    csv_writer.writerow(COLUMN)

    while True:
        strings = f_src.readline()
        if not strings:
            break

        index = strings.find(('CSI_DATA'))
        if index != -1:
            try:
                row_csi = extract_csi_data(strings[index:], 25)
                csv_writer.writerow(row_csi)
            except ValueError:
                continue

    f_src.close()
    f_dst.close()
