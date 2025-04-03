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

# Reduce displayed waveforms to avoid display freezes
CSI_VAID_SUBCARRIER_INTERVAL = 1

csi_vaid_subcarrier_index = []
csi_vaid_subcarrier_color = []
color_step = 255 // (32 // CSI_VAID_SUBCARRIER_INTERVAL + 1)
color_step_57 = 255 // (57 // CSI_VAID_SUBCARRIER_INTERVAL + 1)

csi_vaid_subcarrier_index += [i for i in range(0, 32, CSI_VAID_SUBCARRIER_INTERVAL)]    
csi_vaid_subcarrier_color += [(i * color_step, 0, 0) for i in range(1,  32 // CSI_VAID_SUBCARRIER_INTERVAL + 1)]

csi_vaid_subcarrier_index += [i for i in range(32, 64, CSI_VAID_SUBCARRIER_INTERVAL)]   
csi_vaid_subcarrier_color += [(0, i * color_step, 0) for i in range(1,  32 // CSI_VAID_SUBCARRIER_INTERVAL + 1)]
CSI_DATA_128_COLUMNS = len(csi_vaid_subcarrier_index)

csi_vaid_subcarrier_index += [i for i in range(64, 96, CSI_VAID_SUBCARRIER_INTERVAL)]    
csi_vaid_subcarrier_color += [(0, 0, i * color_step) for i in range(1,  32 // CSI_VAID_SUBCARRIER_INTERVAL + 1)]
csi_vaid_subcarrier_index += [i for i in range(96, 128, CSI_VAID_SUBCARRIER_INTERVAL)]    
csi_vaid_subcarrier_color += [(0, 0, i * color_step) for i in range(1,  32 // CSI_VAID_SUBCARRIER_INTERVAL + 1)]
CSI_DATA_256_COLUMNS = len(csi_vaid_subcarrier_index)
CSI_DATA_INDEX = 200  # buffer size
CSI_DATA_COLUMNS = len(csi_vaid_subcarrier_index)
DATA_COLUMNS_NAMES = ["type", "id", "mac", "rssi", "rate","noise_floor","fft_gain","agc_gain", "channel", "local_timestamp",  "sig_len", "rx_state", "len", "first_word", "data"]
csi_data_array = np.zeros(
    [CSI_DATA_INDEX, CSI_DATA_COLUMNS], dtype=np.float64)
csi_data_phase = np.zeros([CSI_DATA_INDEX, CSI_DATA_COLUMNS], dtype=np.float64)
class csi_data_graphical_window(QWidget):
    def __init__(self):
        super().__init__()

        # 设置窗口大小为 1280x720
        self.resize(1280, 1080)

        # 创建并设置第一个 PlotWidget 用于展示 CSI 行数据
        self.plotWidget_ted = PlotWidget(self)
        self.plotWidget_ted.setGeometry(QtCore.QRect(0, 0, 1280, 360))
        self.plotWidget_ted.setYRange(-20, 100)
        self.plotWidget_ted.addLegend()

        # 初始化 CSI 幅度数据（假设 csi_data_array 是 NumPy 数组）
        self.csi_amplitude_array = csi_data_array  # np.abs(csi_data_array)
        self.csi_phase_array = csi_data_phase
        # 创建并初始化曲线，用于展示 CSI 行数据（红色）
        self.curve = self.plotWidget_ted.plot([], name="CSI Row Data", pen='r')
        self.curve2 = self.plotWidget_ted.plot([], name="CSI Row Data", pen='y')

        # 创建并设置第二个 PlotWidget 用于展示多条子载波的幅度数据
        self.plotWidget_multi_data = PlotWidget(self)
        self.plotWidget_multi_data.setGeometry(QtCore.QRect(0, 360, 1280, 360))  # 下半部分
        self.plotWidget_multi_data.setYRange(-20, 100)
        self.plotWidget_multi_data.addLegend()

        # 初始化曲线列表，用于存储每条子载波的曲线
        self.curve_list = []

        # 假设 csi_vaid_subcarrier_color 是包含每条子载波颜色的列表
        for i in range(CSI_DATA_COLUMNS):
            # 为每条子载波创建曲线，并设置颜色
            curve = self.plotWidget_multi_data.plot(
                self.csi_amplitude_array[:, i], name=str(i), pen=csi_vaid_subcarrier_color[i])
            self.curve_list.append(curve)

        # 显示相位
        self.plotWidget_phase_data = PlotWidget(self)
        self.plotWidget_phase_data.setGeometry(QtCore.QRect(0, 720, 1280, 360))  # 在窗口下半部分显示
        self.plotWidget_phase_data.setYRange(-np.pi, np.pi)  # 设置Y轴范围为相位范围（-π 到 π）
        self.plotWidget_phase_data.addLegend()

        # 初始化曲线列表用于展示相位数据
        self.curve_phase_list = []

        # 生成相位波形的曲线
        for i in range(CSI_DATA_COLUMNS):
            # 为每条子载波的相位数据创建曲线
            phase_curve = self.plotWidget_phase_data.plot(
                np.angle(self.csi_amplitude_array[:, i]), name=str(i), pen=csi_vaid_subcarrier_color[i])
            self.curve_phase_list.append(phase_curve)
        
        # 设置定时器，每 100 毫秒调用一次 update_data 方法更新数据
        self.timer = pq.QtCore.QTimer()
        self.timer.timeout.connect(self.update_data)
        self.timer.start(100)


    def update_data(self):
        # 更新幅度数据的最后一行
        # self.csi_row_data = self.csi_amplitude_array[-1, :]
        # self.curve.setData(self.csi_row_data)  # 更新幅度曲线
        

        # 更新幅度数据的多条子载波曲线
        self.csi_amplitude_array = csi_data_array
        self.csi_phase_array = np.unwrap(np.concatenate((csi_data_phase[:,6:63], csi_data_phase[:,66:123]+np.pi / 2), axis=1))
        # self.csi_phase_array = np.unwrap(np.concatenate((csi_data_phase[:, 66:123], csi_data_phase[:, 6:63]), axis=1))
    
        self.csi_row_data = self.csi_phase_array[-1, :]
        self.curve.setData(self.csi_row_data)  # 更新幅度曲线
        self.csi_row_data = self.csi_phase_array[-1, :] + np.pi/2
        self.curve2.setData(self.csi_row_data)  # 更新幅度曲线

        for i in range(CSI_DATA_COLUMNS):
            self.curve_list[i].setData(self.csi_amplitude_array[:, i])  # 更新幅度曲线
            self.curve_phase_list[i].setData(self.csi_phase_array[:, i])  # 更新相位曲线    

def csi_data_read_parse(port: str, csv_writer, log_file_fd):
    global fft_gains, agc_gains, count, fft_gain_mode, agc_gain_mode, agc_gain, fft_gain
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
            # Save serial output other than CSI data
            log_file_fd.write(strings + '\n')
            log_file_fd.flush()
            continue

        csv_reader = csv.reader(StringIO(strings))
        csi_data = next(csv_reader)

        if len(csi_data) != len(DATA_COLUMNS_NAMES):
            print("element number is not equal")
            log_file_fd.write("element number is not equal\n")
            log_file_fd.write(strings + '\n')
            log_file_fd.flush()
            continue

        try:
            csi_raw_data = json.loads(csi_data[-1])
        except json.JSONDecodeError:
            print("data is incomplete")
            log_file_fd.write("data is incomplete\n")
            log_file_fd.write(strings + '\n')
            log_file_fd.flush()
            continue
        
        if len(csi_raw_data) != 128 and len(csi_raw_data) != 256 and len(csi_raw_data) != 384 and len(csi_raw_data) != 106:
            print(f"element number is not equal: {len(csi_raw_data)}")
            log_file_fd.write(f"element number is not equal: {len(csi_raw_data)}\n")
            log_file_fd.write(strings + '\n')
            log_file_fd.flush()
            continue

        csv_writer.writerow(csi_data)

        # Rotate data to the left
        csi_data_array[:-1] = csi_data_array[1:]
        csi_data_phase[:-1] = csi_data_phase[1:]

        if len(csi_raw_data) == 128 :
            csi_vaid_subcarrier_len = CSI_DATA_128_COLUMNS
        elif  len(csi_raw_data) == 256 :
            csi_vaid_subcarrier_len = CSI_DATA_256_COLUMNS
        else :
            csi_vaid_subcarrier_len = CSI_DATA_COLUMNS

        for i in range(csi_vaid_subcarrier_len):
            csi_data_phase[-1][i] = np.angle(complex(csi_raw_data[csi_vaid_subcarrier_index[i] * 2 + 1],
                                           csi_raw_data[csi_vaid_subcarrier_index[i] * 2]))

            csi_data_array[-1][i] = np.abs(complex(csi_raw_data[csi_vaid_subcarrier_index[i] * 2 + 1],
                                            csi_raw_data[csi_vaid_subcarrier_index[i] * 2]))
        # csi_data_phase[-1] = np.unwrap(csi_data_phase[-1])
    ser.close()
    return


class SubThread (QThread):
    def __init__(self, serial_port, save_file_name, log_file_name):
        super().__init__()
        self.serial_port = serial_port

        save_file_fd = open(save_file_name, 'w')
        self.log_file_fd = open(log_file_name, 'w')
        self.csv_writer = csv.writer(save_file_fd)
        self.csv_writer.writerow(DATA_COLUMNS_NAMES)

    def run(self):
        csi_data_read_parse(self.serial_port, self.csv_writer, self.log_file_fd)

    def __del__(self):
        self.wait()
        self.log_file_fd.close()


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
    parser.add_argument('-l', '--log', dest="log_file", action="store", default="./csi_data_log.txt",
                        help="Save other serial data the bad CSI data to a log file")

    args = parser.parse_args()
    serial_port = args.port
    file_name = args.store_file
    log_file_name = args.log_file

    app = QApplication(sys.argv)

    subthread = SubThread(serial_port, file_name, log_file_name)
    subthread.start()

    window = csi_data_graphical_window()
    window.show()

    sys.exit(app.exec())
