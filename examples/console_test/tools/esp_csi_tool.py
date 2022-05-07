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
import re
import os
import argparse
import pandas as pd
import numpy as np

import serial
from os import path
from os import mkdir
from io import StringIO

from PyQt5.Qt import *
from pyqtgraph import PlotWidget
from PyQt5 import QtCore
import pyqtgraph as pq

import threading
import time
from multiprocessing import Process, Queue

from PyQt5.QtWidgets import QApplication, QMainWindow
from esp_csi_tool_gui import Ui_MainWindow

from scipy import signal
import signal as signal_key

CSI_SAMPLE_RATE = 100

# Remove invalid subcarriers
# secondary channel : below, HT, 40 MHz, non STBC, v, HT-LFT: 0~63, -64~-1, 384
CSI_VAID_SUBCARRIER_INTERVAL = 3
csi_vaid_subcarrier_index = []
csi_vaid_subcarrier_color = []
color_step = 255 // (28 // CSI_VAID_SUBCARRIER_INTERVAL + 1)

# LLTF: 52
csi_vaid_subcarrier_index += [i for i in range(0, 26, CSI_VAID_SUBCARRIER_INTERVAL)]     # 26  red
csi_vaid_subcarrier_color += [(i * color_step, 0, 0) for i in range(1,  26 // CSI_VAID_SUBCARRIER_INTERVAL + 2)]
csi_vaid_subcarrier_index += [i for i in range(26, 52, CSI_VAID_SUBCARRIER_INTERVAL)]    # 26  green
csi_vaid_subcarrier_color += [(0, i * color_step, 0) for i in range(1,  26 // CSI_VAID_SUBCARRIER_INTERVAL + 2)]

# HT-LFT: 56 + 56
# csi_vaid_subcarrier_index += [i for i in range(66, 94, CSI_VAID_SUBCARRIER_INTERVAL)]    # 28  blue
# csi_vaid_subcarrier_color += [(0, 0, i * color_step) for i in range(1,  28 // CSI_VAID_SUBCARRIER_INTERVAL + 2)]
# csi_vaid_subcarrier_index += [i for i in range(95, 123, CSI_VAID_SUBCARRIER_INTERVAL)]   # 28  White
# csi_vaid_subcarrier_color += [(i * color_step, i * color_step, i * color_step) for i in range(1,  28 // CSI_VAID_SUBCARRIER_INTERVAL + 2)]
# csi_vaid_subcarrier_index += [i for i in range(124, 162)]  # 28  White
# csi_vaid_subcarrier_index += [i for i in range(163, 191)]  # 28  White

CSI_DATA_INDEX   = 100  # buffer size
CSI_DATA_COLUMNS = len(csi_vaid_subcarrier_index)
CSI_DATA_COLUMNS_NAMES = ["type", "id", "action_id" ,"action", "mac", "rssi", "rate", "sig_mode", "mcs", "cwb", "smoothing", "not_sounding", "aggregation", "stbc", "fec_coding",
                          "sgi", "noise_floor", "ampdu_cnt", "channel_primary", "channel_secondary", "local_timestamp", "ant", "sig_len", "rx_state", "len", "first_word_invalid", "data"]
CSI_DATA_ACTIONS       = ["unknown" ,"none" ,"someone" ,"static" ,"move" ,"front" ,"after" ,"left" ,"right" ,"go" ,"jump" ,"sit down" ,"stand up" ,"climb up" ,"wave" ,"applause"]
RADAR_DATA_COLUMNS_NAMES = ["type","id", "corr", "corr_max", "corr_min", "corr_threshold", "corr_trigger"]

g_csi_phase_array = np.zeros([CSI_DATA_INDEX, CSI_DATA_COLUMNS], dtype=np.int32)
g_rssi_array      = np.zeros(CSI_DATA_INDEX, dtype=np.int8)
g_radio_header_pd = pd.DataFrame(np.zeros([10, len(CSI_DATA_COLUMNS_NAMES[1:])], dtype=np.int32), columns=CSI_DATA_COLUMNS_NAMES[1:])

RADAR_DATA_INDEX = 100  # buffer size

g_radar_corr_array     = np.zeros(RADAR_DATA_INDEX, dtype=np.float32)
g_radar_corr_min_array = np.zeros(RADAR_DATA_INDEX, dtype=np.float32)
g_radar_corr_max_array = np.zeros(RADAR_DATA_INDEX, dtype=np.float32)
g_radar_corr_threshold_array = np.zeros(RADAR_DATA_INDEX, dtype=np.float32)
g_radar_corr_trigger_array   = np.zeros(RADAR_DATA_INDEX, dtype=np.float32)

class DataGraphicalWindow(QMainWindow, Ui_MainWindow):
    def __init__(self, serial_queue_write, parent=None):
        super(DataGraphicalWindow, self).__init__(parent)
        self.setupUi(self)

        self.curve_subcarrier_range = np.array([10, 20]) 
        self.graphicsView_subcarrier.setYRange(self.curve_subcarrier_range[0], self.curve_subcarrier_range[1])
        self.graphicsView_subcarrier.addLegend()

        self.curve_subcarrier = []
        self.serial_queue_write = serial_queue_write

        for i in range(CSI_DATA_COLUMNS):
            # curve = self.graphicsView_subcarrier.plot(
            #     self.g_csi_phase_array[:, i], name=str(i), pen=csi_vaid_subcarrier_color[i])
            # self.curve_list.append(curve)
            curve = self.graphicsView_subcarrier.plot(g_csi_phase_array[:, i], name=str(i), pen=csi_vaid_subcarrier_color[i])
            self.curve_subcarrier.append(curve)

        self.curve_rssi = self.graphicsView_rssi.plot(g_rssi_array, name='rssi', pen=(255, 255, 255))
        # self.graphicsView_rssi.setYRange(-10, -65)
        
        self.model_radio_header = QStandardItemModel(1, len(g_radio_header_pd.columns.values))
        self.model_radio_header.setHorizontalHeaderLabels(g_radio_header_pd.columns.values)
        self.tableView_radioHeader.setModel(self.model_radio_header)
        self.tableView_values = {
            'sig_mode': ['non HT(11bg)', 'HT(11n)', 'VHT(11ac)'],
            'mcs': [f'MSC{i}' for i in range(76) ],
            "cwb": ['20MHz', '40MHz'],
            'aggregation': ['MPDU', 'AMPDU'], 
            'channel_secondary':['none', 'above', 'below'],
            'stbc': ['non STBC', 'STBC'],
            'sgi': ['Long GI', 'Short GI']
            }

        self.graphicsView_corr.setYRange(0.0, 1.0)
        self.graphicsView_corr.addLegend()

        self.progressBar_radar_status.setValue(0)
        self.lcdNumber_radar_count.display(0)

        self.verticalSlider_corr_threshold.valueChanged.connect(self.label_corr_threshold_show)
        self.verticalSlider_corr_threshold.sliderReleased.connect(self.command_corr_threshold)

        self.pushButton_router_connect.released.connect(self.command_router_connect)
        self.pushButton_ping_start.released.connect(self.command_ping_start)
        self.pushButton_command.released.connect(self.command_custom)

        self.comboBox_command.activated.connect(self.comboBox_command_show)

        self.pushButton_label_start.released.connect(self.pushButton_label_show)
        self.pushButton_calibrate_start.released.connect(self.pushButton_calibrate_show)

        self.curve_radar_corr     = self.graphicsView_corr.plot(g_radar_corr_array, name='corr', pen=(255, 255, 255)) 
        self.curve_radar_corr_max = self.graphicsView_corr.plot(g_radar_corr_max_array, name='corr_max', pen=(128, 0, 0)) 
        self.curve_radar_corr_min = self.graphicsView_corr.plot(g_radar_corr_min_array, name='corr_min', pen=(128, 0, 0)) 
        self.curve_radar_corr_trigger  = self.graphicsView_corr.plot(g_radar_corr_trigger_array, name='status', pen=(0, 0, 255))

        self.textBrowser_log.setStyleSheet("background:black")

        self.timer_curve_subcarrier = QTimer()
        self.timer_curve_subcarrier.timeout.connect(self.show_curve_subcarrier)
        self.timer_curve_subcarrier.start(200)

        self.timer_curve_radar = QTimer()
        self.timer_curve_radar.timeout.connect(self.show_curve_radar)
        self.timer_curve_radar.start(200)

        self.label_number         = self.spinBox_label_number.value()
        self.timer_label_duration = QTimer()
        self.timer_label_duration.timeout.connect(self.spinBox_label_number_show)

        self.label_delay  = self.timeEdit_label_delay.time()
        self.timer_label_delay    = QTimer()
        self.timer_label_delay.setInterval(1000) 
        self.timer_label_delay.timeout.connect(self.timeEdit_label_delay_show)


        self.calibrate_duration       = self.timeEdit_calibrate_duration.time()
        self.timer_calibrate_duration = QTimer()
        self.timer_calibrate_duration.setInterval(1000) 
        self.timer_calibrate_duration.timeout.connect(self.spinBox_calibrate_duration_show)

        self.calibrate_delay          = self.timeEdit_calibrate_delay.time()
        self.timer_calibrate_delay    = QTimer()
        self.timer_calibrate_delay.setInterval(1000) 
        self.timer_calibrate_delay.timeout.connect(self.timeEdit_calibrate_delay_show)

    def show_curve_subcarrier(self):
        wn = 10 / (CSI_SAMPLE_RATE / 2)
        b, a = signal.butter(8, wn, 'lowpass')
        csi_filtfilt_data = signal.filtfilt(b, a, g_csi_phase_array.T).T

        if self.curve_subcarrier_range[0] > csi_filtfilt_data.min() or self.curve_subcarrier_range[1] < csi_filtfilt_data.max():
            if csi_filtfilt_data.min() > 0 and self.curve_subcarrier_range[0] > csi_filtfilt_data.min():
                self.curve_subcarrier_range[0] = csi_filtfilt_data.min() - 1
            if self.curve_subcarrier_range[1] < csi_filtfilt_data.max():
                self.curve_subcarrier_range[1] = csi_filtfilt_data.max() + 1
            self.graphicsView_subcarrier.setYRange(self.curve_subcarrier_range[0], self.curve_subcarrier_range[1])

        for i in range(CSI_DATA_COLUMNS):
            self.curve_subcarrier[i].setData(csi_filtfilt_data[:, i])
            # self.curve_subcarrier[i].setData(g_csi_phase_array[:, i])

        csi_filtfilt_rssi = signal.filtfilt(b, a, g_rssi_array).astype(np.int32)
        self.curve_rssi.setData(csi_filtfilt_rssi)
        # self.curve_rssi.setData(g_rssi_array)

        for i in range(g_radio_header_pd.shape[0]):
            for j in range(g_radio_header_pd.shape[1]):
                if g_radio_header_pd.columns.values[j] in self.tableView_values.keys():
                    # print(j, g_radio_header_pd.columns.values[j], self.tableView_values[g_radio_header_pd.columns.values[j]], g_radio_header_pd.iloc[i,j])
                    str_values = self.tableView_values[g_radio_header_pd.columns.values[j]][int(g_radio_header_pd.iloc[i,j])]
                    item = QStandardItem(str_values)
                else :
                    # print(j, g_radio_header_pd.columns.values[j])
                    item = QStandardItem(g_radio_header_pd.iloc[i,j])
                self.model_radio_header.setItem(i, j, item)

    def show_curve_radar(self):
        self.curve_radar_corr.setData(g_radar_corr_array)
        self.curve_radar_corr_min.setData(g_radar_corr_min_array)
        self.curve_radar_corr_max.setData(g_radar_corr_max_array)
        self.curve_radar_corr_trigger.setData(g_radar_corr_trigger_array)

    def label_corr_threshold_show(self):
        self.label_corr_threshold.setText(str(self.verticalSlider_corr_threshold.value() * 0.01)[:4])

    def command_corr_threshold(self):
        self.serial_queue_write.put("radar --corr_threshold " + str(self.verticalSlider_corr_threshold.value() * 0.01)[:4])

    def command_router_connect(self):
        commmand = "wifi_config --ssid " + str(self.lineEdit_router_ssid.text()) + " --password " + str(self.lineEdit_router_password.text())
        self.serial_queue_write.put(commmand)

    def command_ping_start(self):
        if self.pushButton_ping_start.text() == "start":
            commmand = "ping --interval " + str(self.spinBox_ping_interval.text()) + " --count " + str(self.spinBox_ping_count.text())
            self.pushButton_ping_start.setText("stop")
            self.pushButton_ping_start.setStyleSheet("color: red")
        else:
            commmand = "ping --abort"
            self.pushButton_ping_start.setText("start")
            self.pushButton_ping_start.setStyleSheet("color: black")
        self.serial_queue_write.put(commmand)

    def command_custom(self):
        commmand = self.lineEdit_command.text()
        self.serial_queue_write.put(commmand)

    def command_label_action_start(self):
        commmand = "radar --id " + str(self.spinBox_label_number.value()) + " --action " + str(self.comboBox_label_action.currentIndex())
        self.serial_queue_write.put(commmand)
    
    def command_label_action_stop(self):
        commmand = "radar --id 0 --action 0"
        self.serial_queue_write.put(commmand)

    def spinBox_label_number_show(self):
        number = self.spinBox_label_number.value()
        if number >= 1:
            self.spinBox_label_number.setValue(number - 1)
            self.timer_label_duration.start()
            self.command_label_action_start()
        else:
            self.timer_label_duration.stop()
            self.command_label_action_stop()

            self.spinBox_label_number.setValue(self.label_number)
            self.timeEdit_label_delay.setTime(self.label_delay)
            self.pushButton_label_start.setStyleSheet("color: black")
            self.spinBox_label_number.setStyleSheet("color: black")
            self.pushButton_label_start.setText("start")

    def timeEdit_label_delay_show(self):
        time_temp = self.timeEdit_label_delay.time()
        second = time_temp.hour() * 3600 + time_temp.minute() * 60 + time_temp.second()
        if second > 0:
            self.timeEdit_label_delay.setTime(time_temp.addSecs(-1))
            self.timer_label_delay.start()
        else:
            self.timer_label_delay.stop()

            duration = self.spinBox_label_duration.value()
            self.timer_label_duration.setInterval(duration) 
            self.timer_label_duration.start()
            self.command_label_action_start()
            self.spinBox_label_number.setStyleSheet("color: red")
            self.timeEdit_label_delay.setStyleSheet("color: black")

    def pushButton_label_show(self):
        if self.pushButton_label_start.text() == "start":
            print(self.comboBox_label_action.currentIndex())
            if self.comboBox_label_action.currentIndex() == 0 or self.spinBox_label_number.value() == 0:
                err = QErrorMessage(self)
                err.setWindowTitle('Label parameter error')
                err.showMessage("Please check whether 'action' or 'number' is set")
                err.show()
                return

            self.pushButton_label_start.setText("stop")
            self.pushButton_label_start.setStyleSheet("color: red")
            self.timeEdit_label_delay.setStyleSheet("color: red")
            self.timer_label_delay.start()
            self.label_number = self.spinBox_label_number.value()
            self.label_delay  = self.timeEdit_label_delay.time()
        else:
            self.spinBox_label_number.setValue(self.label_number)
            self.timeEdit_label_delay.setTime(self.label_delay)
            self.spinBox_label_number.setStyleSheet("color: black")
            self.pushButton_label_start.setStyleSheet("color: black")
            self.timer_label_delay.stop()
            self.timer_label_duration.stop()
            self.command_label_action_stop()
            self.pushButton_label_start.setText("start")

    def command_calibrate_action_start(self):
        action =  self.comboBox_calibrate_action.currentText()
        commmand = "radar --mode 1 --action " + str(CSI_DATA_ACTIONS.index(action))
        self.serial_queue_write.put(commmand)
    
    def command_calibrate_action_stop(self):
        commmand = "radar --mode 1 --action 0"
        self.serial_queue_write.put(commmand)

    def spinBox_calibrate_duration_show(self):
        time_temp = self.timeEdit_calibrate_duration.time()
        second = time_temp.hour() * 3600 + time_temp.minute() * 60 + time_temp.second()
        if second > 0:
            self.timeEdit_calibrate_duration.setTime(time_temp.addSecs(-1))
            self.timer_calibrate_duration.start()
        else:
            self.timer_calibrate_duration.stop()
            self.command_calibrate_action_stop()
            self.timeEdit_calibrate_delay.setTime(self.calibrate_delay)
            self.timeEdit_calibrate_duration.setTime(self.calibrate_duration)
            self.timeEdit_calibrate_duration.setStyleSheet("color: black")
            self.pushButton_calibrate_start.setText("start")
            self.pushButton_calibrate_start.setStyleSheet("color: black")

    def timeEdit_calibrate_delay_show(self):
        time_temp = self.timeEdit_calibrate_delay.time()
        second = time_temp.hour() * 3600 + time_temp.minute() * 60 + time_temp.second()
        if second > 0:
            self.timeEdit_calibrate_delay.setTime(time_temp.addSecs(-1))
            self.timer_calibrate_delay.start()
        else:
            self.timer_calibrate_delay.stop()
            self.command_calibrate_action_start()
            self.spinBox_calibrate_duration_show()
            self.timeEdit_calibrate_duration.setStyleSheet("color: red")
            self.timeEdit_calibrate_delay.setStyleSheet("color: black")

    def pushButton_calibrate_show(self):
        if self.pushButton_calibrate_start.text() == "start":
            self.calibrate_delay    = self.timeEdit_calibrate_delay.time()
            self.calibrate_duration = self.timeEdit_calibrate_duration.time()
            self.pushButton_calibrate_start.setText("stop")
            self.pushButton_calibrate_start.setStyleSheet("color: red")
            self.timeEdit_calibrate_delay.setStyleSheet("color: red")

            self.timeEdit_calibrate_delay_show();
        else:
            self.timer_calibrate_delay.stop()
            self.timer_calibrate_duration.stop()
            self.command_calibrate_action_stop()
            self.timeEdit_calibrate_delay.setTime(self.calibrate_delay)
            self.timeEdit_calibrate_duration.setTime(self.calibrate_duration)
            self.pushButton_calibrate_start.setText("start")
            self.pushButton_calibrate_start.setStyleSheet("color: black")

    def comboBox_command_show(self):
        self.lineEdit_command.setText(self.comboBox_command.currentText())

    def progressBar_radar_status_show(self, status):
        # print('-' * 10, status)
        self.progressBar_radar_status.setValue(status)

    def lcdNumber_radar_count_show(self, count):
        # print("count: ", count)
        self.lcdNumber_radar_count.display(count)

    def textBrowser_log_show(self, str):
        self.textBrowser_log.append(str)
        # self.textBrowser_log.moveCursor(self.textBrowser_log.textCursor().End)

    def closeEvent(self, event):
        self.serial_queue_write.put("exit")
        time.sleep(0.5)

        event.accept()

        try:
            os._exit(0)
        except Exception as e:
            print(f"GUI closeEvent: {e}")

def csi_data_handle(self, data):
    csi_raw_data = json.loads(data['data'])
    
    # Rotate data
    g_csi_phase_array[:-1]     = g_csi_phase_array[1:]
    g_rssi_array[:-1]          = g_rssi_array[1:]
    g_radio_header_pd.iloc[1:] = g_radio_header_pd.iloc[:-1]

    for i in range(CSI_DATA_COLUMNS):
        data_complex = complex(csi_raw_data[csi_vaid_subcarrier_index[i] * 2],
                               csi_raw_data[csi_vaid_subcarrier_index[i] * 2 - 1])
        g_csi_phase_array[-1][i] = np.abs(data_complex)

    g_rssi_array[-1]         = data['rssi']
    g_radio_header_pd.loc[0] = data[1:]

    return


def radar_data_handle(self, data):
    if int(data['corr_trigger']) > 0:
        self.action_count += 1
    self.signal_progressBar_radar_status.emit(int(int(data['corr_trigger']) > 0) * 100)
    self.signal_lcdNumber_radar_count.emit(self.action_count)

    g_radar_corr_array[:-1] = g_radar_corr_array[1:]
    g_radar_corr_array[-1]  = data['corr']

    g_radar_corr_max_array[:-1] = g_radar_corr_max_array[1:]
    g_radar_corr_max_array[-1]  = data['corr_max']

    g_radar_corr_min_array[:-1] = g_radar_corr_min_array[1:]
    g_radar_corr_min_array[-1]  = data['corr_min']

    g_radar_corr_threshold_array[:-1] = g_radar_corr_threshold_array[1:]
    g_radar_corr_threshold_array[-1]  = data['corr_threshold']

    g_radar_corr_trigger_array[:-1] = g_radar_corr_trigger_array[1:]
    g_radar_corr_trigger_array[-1]  = data['corr_trigger']

class DataHandleThread(QThread):
    signal_log_msg = pyqtSignal(str)
    signal_progressBar_radar_status = pyqtSignal(int)
    signal_lcdNumber_radar_count    = pyqtSignal(int)
    signal_exit                     = pyqtSignal()

    def __init__(self, serial_queue_read):
        super().__init__()

        self.serial_queue_read  = serial_queue_read
        self.action_count       = 0

    def run(self):
        while True:
            series = self.serial_queue_read.get()
            if series['type'] == 'CSI_DATA':
                csi_data_handle(self, series)
            elif series['type'] == 'RADAR_DADA':
                radar_data_handle(self, series)
            elif series['type'] == 'LOG_DATA':
                if series['tag'] == 'I':
                    color = "green"
                elif series['tag'] == 'W':
                    color = "yellow"
                elif series['tag'] == 'E':
                    color = "red"
                else:
                    color = "white"

                data = "<font color=\'%s\'>%s (%s) %s <font>" % (color, series['tag'], series['timestamp'] ,series['data'])
                self.signal_log_msg.emit(data)
            elif series['type'] == 'FAIL_EVENT':
                print(f"Fial: {series['data']}")

                self.signal_exit.emit()
                time.sleep(1)
                sys.exit(0)
            else:
                print("error: ", series)

            QApplication.processEvents()

def serial_handle(queue_read, queue_write, port):
    try:
        ser = serial.Serial(port=port, baudrate=921600,
                    bytesize=8, parity='N', stopbits=1, timeout=0.1)
    except Exception as e:
        print(f"serial_handle: {e}")
        data_series = pd.Series(index = ['type', 'data'],
                                data = ['FAIL_EVENT',"Failed to open serial port"])
        queue_read.put(data_series)
        sys.exit()
        return

    print("open serial port: ", port)

    # Wait a second to let the port initialize
    ser.flushInput()

    folder_list = ['log', 'data']
    for folder in folder_list:
        if not path.exists(folder):
            mkdir(folder)

    data_valid_list = pd.DataFrame(columns = ['type', 'columns_names', 'file_name', 'file_fd'],
                                    data = [["CSI_DATA",CSI_DATA_COLUMNS_NAMES, "log/csi_data.csv",'NULL'], 
                                            ["RADAR_DADA",RADAR_DATA_COLUMNS_NAMES, "log/radar_data.csv",'NULL']])

    for data_valid in data_valid_list.iloc:
        # print(type(data_valid), data_valid)
        # print(f"file_name: {data_valid['file_name']}")
        data_file_fd = open(data_valid['file_name'], 'w')
        data_csv_writer = csv.writer(data_file_fd)
        data_csv_writer.writerow(data_valid['columns_names'])
        data_valid['file_fd'] = data_csv_writer

    log_data_writer = open("log/log_data.txt", 'w+')
    action_data_writer = 'null'
    action_last = '0'
    action_id_last = '0'

    while True:
        if not queue_write.empty():
            command = queue_write.get()
            if command == "exit":
                sys.exit()

            command = command + "\r\n"
            print("serial write: ", command)
            ser.write(command.encode('utf-8'))

        try:
            strings = str(ser.readline())

            if not strings:
               continue
        except Exception as e:
            data_series = pd.Series(index = ['type', 'data'],
                                    data  = ['FAIL_EVENT',"Failed to read serial"])
            queue_read.put(data_series)
            sys.exit()

        strings = strings.lstrip('b\'').rstrip('\\r\\n\'')

        for data_valid in data_valid_list.iloc:
            index = strings.find(data_valid['type'])
            if index >= 0:
                strings = strings[index:]
                csv_reader = csv.reader(StringIO(strings))
                data = next(csv_reader)

                if len(data) == len(data_valid['columns_names']):
                    data_valid['file_fd'].writerow(data)
                    data_series = pd.Series(data, index = data_valid['columns_names'])
                    if data_series['type'] == 'CSI_DATA':
                        try:
                            csi_raw_data = json.loads(data_series['data'])
                            if len(csi_raw_data) != int(data_series['len']):
                                # if len(csi_raw_data) != 104 and len(csi_raw_data) != 216 and len(csi_raw_data) != 328 and len(csi_raw_data) != 552:
                                print(f"CSI_DATA, expected: {data_series['len']}, actual: {len(csi_raw_data)}")
                                break
                        except Exception as e:
                            print(f"json.JSONDecodeError: {e}, data: {data_series['data']}")
                            break

                        if data_series['action'] != '0':
                            if data_series['action'] != action_last or data_series['action_id'] != action_id_last:
                                folder = f"data/{CSI_DATA_ACTIONS[int(data_series['action'])]}"
                                if not path.exists(folder):
                                    mkdir(folder)

                                csi_action_data_file_name = f"{folder}/{time.strftime('%Y-%m-%d-%H-%M-%S', time.localtime())}_{data_series['len']}_{data_series['action_id']}.csv"
                                # print(csi_action_data_file_name)
                                csi_action_data_file_fd   = open(csi_action_data_file_name, 'w')
                                action_data_writer = csv.writer(csi_action_data_file_fd)
                            action_data_writer.writerow(csi_raw_data)

                        action_last = data_series['action']
                        action_id_last = data_series['action_id']

                    if queue_read.full():
                        # print('============== queue_full ==========')
                        pass
                    else:
                        queue_read.put(data_series)
                    break
                break
        else:
            strings = re.sub(r'\\x1b.*?m', '', strings)
            log = re.match( r'.*([DIWE]) \((\d+)\) (.*)', strings, re.I)

            if not log:
                # print("Fail: ", strings)
                continue

            # print(strings)
            data_series = pd.Series(index = ['type', 'tag', 'timestamp', 'data'],
                                    data = ['LOG_DATA',log.group(1), log.group(2), log.group(3)])
            if not queue_read.full():
                queue_read.put(data_series)
            log_data_writer.writelines(strings + "\n")

def quit(signum, frame):
    print("Exit the system")
    sys.exit()

if __name__ == '__main__':
    if sys.version_info < (3, 6):
        print(" Python version should >= 3.6")
        exit()

    parser = argparse.ArgumentParser(
        description="Read CSI data from serial port and display it graphically")
    parser.add_argument('-p', '--port', dest='port', action='store', required=True,
                        help="Serial port number of csv_recv device")

    args = parser.parse_args()
    serial_port = args.port

    serial_queue_read  = Queue(maxsize=32)
    serial_queue_write = Queue(maxsize=32)

    signal_key.signal(signal_key.SIGINT, quit)                                
    signal_key.signal(signal_key.SIGTERM, quit)

    serial_handle_process = Process(target=serial_handle, args=(serial_queue_read, serial_queue_write, serial_port))
    serial_handle_process.start()

    app = QApplication(sys.argv)
    app.setWindowIcon(QIcon('../../../docs/_static/icon.png'))

    window = DataGraphicalWindow(serial_queue_write)
    data_handle_thread = DataHandleThread(serial_queue_read)
    data_handle_thread.signal_log_msg.connect(window.textBrowser_log_show)
    data_handle_thread.signal_progressBar_radar_status.connect(window.progressBar_radar_status_show)
    data_handle_thread.signal_lcdNumber_radar_count.connect(window.lcdNumber_radar_count_show)
    data_handle_thread.signal_exit.connect(window.close)
    data_handle_thread.start()

    window.show()
    sys.exit(app.exec())
    serial_handle_process.join()
