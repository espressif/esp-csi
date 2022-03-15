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
import csv
import json
import argparse
import pandas as pd
import numpy as np

import serial
from os import path
from io import StringIO

from PyQt5.Qt import *
from pyqtgraph import PlotWidget
from PyQt5 import QtCore
import pyqtgraph as pq

import threading
import time

# Remove invalid subcarriers
csi_vaid_subcarrier_index = []
csi_vaid_subcarrier_index += [i for i in range(4, 27)]     # 23  red
csi_vaid_subcarrier_index += [i for i in range(39, 60)]    # 21  green
csi_vaid_subcarrier_index += [i for i in range(68, 90)]    # 22  blue
csi_vaid_subcarrier_index += [i for i in range(101, 124)]  # 23  White

CSI_DATA_INDEX = 200  # buffer size
CSI_DATA_COLUMNS = len(csi_vaid_subcarrier_index)
DATA_COLUMNS_NAMES = ["type", "id", "mac", "rssi", "rate", "sig_mode", "mcs", "bandwidth", "smoothing", "not_sounding", "aggregation", "stbc", "fec_coding",
                      "sgi", "noise_floor", "ampdu_cnt", "channel", "secondary_channel", "local_timestamp", "ant", "sig_len", "rx_state", "len", "first_word", "data"]
csi_data_array = np.zeros(
    [CSI_DATA_INDEX, CSI_DATA_COLUMNS], dtype=np.complex64)


class csi_data_graphical_window(QWidget):
    def __init__(self):
        super().__init__()

        self.resize(1280, 720)
        self.plotWidget_ted = PlotWidget(self)
        self.plotWidget_ted.setGeometry(QtCore.QRect(0, 0, 1280, 720))

        self.plotWidget_ted.setYRange(-20, 100)
        self.plotWidget_ted.addLegend()

        self.csi_phase_array = np.abs(csi_data_array)
        self.curve_list = []

        for i in range(CSI_DATA_COLUMNS):
            r = 0
            g = 0
            b = 0

            if i < 23:
                r = i * 8 + 55
            elif i < 23 + 21:
                g = (i - 23) * 8 + 55
            elif i < 23 + 21 + 22:
                b = (i - (23 + 21)) * 8 + 40
            else:
                r = (i - (23 + 22 + 21)) * 8 + 55
                g = (i - (23 + 22 + 21)) * 8 + 55
                b = (i - (23 + 22 + 21)) * 8 + 55

            curve = self.plotWidget_ted.plot(
                self.csi_phase_array[:, i], name=str(i), pen=(r, g, b))
            self.curve_list.append(curve)

        self.timer = pq.QtCore.QTimer()
        self.timer.timeout.connect(self.update_data)
        self.timer.start(100)

    def update_data(self):
        self.csi_phase_array = np.abs(csi_data_array)

        for i in range(CSI_DATA_COLUMNS):
            self.curve_list[i].setData(self.csi_phase_array[:, i])


def csi_data_read_parse(port: str, csv_writer):
    ser = serial.Serial(port=port, baudrate=921600,
                        bytesize=8, parity='N', stopbits=1)
    if ser.isOpen():
        print("open success")
    else:
        print("open failed")
        return

    while True:
        strings = str(ser.readline())
        if not strings:
            break

        strings = strings.lstrip('b\'').rstrip('\\r\\n\'')
        index = strings.find('CSI_DATA')

        if index == -1:
            continue

        csv_reader = csv.reader(StringIO(strings))
        csi_data = next(csv_reader)

        if len(csi_data) != len(DATA_COLUMNS_NAMES):
            print("element number is not equal")
            continue

        try:
            csi_raw_data = json.loads(csi_data[-1])
        except json.JSONDecodeError:
            print(f"data is not incomplete")
            continue

        if len(csi_raw_data) != 256:
            print("element number is not equal")
            continue

        csv_writer.writerow(csi_data)

        # Rotate data to the left
        csi_data_array[:-1] = csi_data_array[1:]

        for i in range(CSI_DATA_COLUMNS):
            csi_data_array[-1][i] = complex(csi_raw_data[csi_vaid_subcarrier_index[i] * 2],
                                            csi_raw_data[csi_vaid_subcarrier_index[i] * 2 - 1])

    ser.close()
    return


class SubThread (QThread):
    def __init__(self, serial_port, save_file_name):
        super().__init__()
        self.serial_port = serial_port

        save_file_fd = open(save_file_name, 'w')
        self.csv_writer = csv.writer(save_file_fd)
        self.csv_writer.writerow(DATA_COLUMNS_NAMES)

    def run(self):
        csi_data_read_parse(self.serial_port, self.csv_writer)

    def __del__(self):
        self.wait()


if __name__ == '__main__':
    if sys.version_info < (3, 6):
        print(" Python version should >= 3.6")
        exit()

    parser = argparse.ArgumentParser(
        description="Read CSI data from serial port and display it graphically")
    parser.add_argument('-p', '--port', dest='port', action='store', required=True,
                        help="Serial port number of csv_recv device")
    parser.add_argument('-s', '--store', dest='store_file', action='store', default='./csi_data.csv',
                        help="Save the data printed by the serial port to a file")

    args = parser.parse_args()
    serial_port = args.port
    file_name = args.store_file

    app = QApplication(sys.argv)

    subthread = SubThread(serial_port, file_name)
    subthread.start()

    window = csi_data_graphical_window()
    window.show()

    sys.exit(app.exec())
