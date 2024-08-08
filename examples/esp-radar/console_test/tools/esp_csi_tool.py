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

from PyQt5.QtCore import QDate, QDate, QTime, QDateTime

import threading
import base64
import time
from datetime import datetime
from multiprocessing import Process, Queue

from PyQt5.QtWidgets import QApplication, QMainWindow, QMessageBox
# from PyQt5.QtChart import QChart, QLineSeries,QValueAxis
from esp_csi_tool_gui import Ui_MainWindow

from scipy import signal
import signal as signal_key
import socket


CSI_SAMPLE_RATE = 100

# Remove invalid subcarriers
# secondary channel : below, HT, 40 MHz, non STBC, v, HT-LFT: 0~63, -64~-1, 384
CSI_VAID_SUBCARRIER_INTERVAL = 5
csi_vaid_subcarrier_index = []
csi_vaid_subcarrier_color = []
color_step = 255 // (28 // CSI_VAID_SUBCARRIER_INTERVAL + 1)

# LLTF: 52
# 26  red
csi_vaid_subcarrier_index += [i for i in range(
    0, 26, CSI_VAID_SUBCARRIER_INTERVAL)]
csi_vaid_subcarrier_color += [(i * color_step, 0, 0)
                              for i in range(1,  26 // CSI_VAID_SUBCARRIER_INTERVAL + 2)]
# 26  green
csi_vaid_subcarrier_index += [i for i in range(
    26, 52, CSI_VAID_SUBCARRIER_INTERVAL)]
csi_vaid_subcarrier_color += [(0, i * color_step, 0)
                              for i in range(1,  26 // CSI_VAID_SUBCARRIER_INTERVAL + 2)]

DEVICE_INFO_COLUMNS_NAMES = ["type", "timestamp", "compile_time", "chip_name", "chip_revision",
                             "app_revision", "idf_revision", "total_heap", "free_heap", "router_ssid", "ip", "port"]
g_device_info_series = None

CSI_DATA_INDEX = 500  # buffer size
CSI_DATA_COLUMNS = len(csi_vaid_subcarrier_index)
CSI_DATA_COLUMNS_NAMES = ["type", "seq", "timestamp", "taget_seq", "taget", "mac", "rssi", "rate", "sig_mode", "mcs",
                          "cwb", "smoothing", "not_sounding", "aggregation", "stbc", "fec_coding","sgi", "noise_floor", 
                          "ampdu_cnt", "channel_primary", "channel_secondary", "local_timestamp", "ant", "sig_len", 
                          "rx_state", "agc_gain", "fft_gain", "len", "first_word_invalid", "data"]
CSI_DATA_TARGETS = ["unknown", "train", "none", "someone", "static", "move", "front",
                    "after", "left", "right", "go", "jump", "sit down", "stand up", "climb up", "wave", "applause"]
RADAR_DATA_COLUMNS_NAMES = ["type", "seq", "timestamp",
                            "waveform_wander", "wander_average", "waveform_wander_threshold", "someone_status", 
                            "waveform_jitter", "jitter_midean", "waveform_jitter_threshold", "move_status"]

g_csi_amplitude_array = np.zeros(
    [CSI_DATA_INDEX, CSI_DATA_COLUMNS], dtype=np.int32)
g_rssi_array = np.zeros(CSI_DATA_INDEX, dtype=np.int8)
g_radio_header_pd = pd.DataFrame(np.zeros([10, len(
    CSI_DATA_COLUMNS_NAMES[1:-1])], dtype=np.int32), columns=CSI_DATA_COLUMNS_NAMES[1:-1])

RADAR_STATUS_RECORD = ["room", "human", "spend_time", "start_time", "stop_time"]
g_status_record_pd = pd.DataFrame(np.zeros(
    [20, len(RADAR_STATUS_RECORD)], dtype=np.str_), columns=RADAR_STATUS_RECORD)

RADAR_MOVE_RECORD = ["date", "hour", "minute", "second", "count"]
g_move_record_pd = pd.DataFrame(columns=RADAR_MOVE_RECORD)

RADAR_DATA = ["status", "threshold", "value", "max", "min", "mean", "std"]
g_radar_data_room_pd = pd.DataFrame(
    np.zeros([10, len(RADAR_DATA)], dtype=np.float32), columns=RADAR_DATA)
g_radar_data_human_pd = pd.DataFrame(
    np.zeros([10, len(RADAR_DATA)], dtype=np.float32), columns=RADAR_DATA)

g_current_time = QDateTime.currentDateTime()

RADAR_DATA_INDEX = 100  # buffer size

ROOM_STATUS_NAMES = ['none', 'someone']
HUMAN_STATUS_NAMES = ['static', 'move']
RADAR_WAVEFORM_NAMES = ["wander", "jitter"]
RADAR_targetS_NAMES = ["someone", "move"]
RADAR_targetS_LEN = len(RADAR_targetS_NAMES)
g_radar_eigenvalue_color = [(0, 0, 255), (0, 255, 0)]
g_radar_eigenvalue_threshold_color = [(255, 255, 0), (255, 0, 255)]
g_radar_eigenvalue_array = np.zeros(
    [RADAR_DATA_INDEX, RADAR_targetS_LEN], dtype=np.float32)
g_radar_eigenvalue_threshold_array = np.zeros(
    [RADAR_DATA_INDEX, RADAR_targetS_LEN], dtype=np.float32)
g_radar_status_array = np.zeros(
    [RADAR_DATA_INDEX, RADAR_targetS_LEN], dtype=np.int32)
g_evaluate_statistics_array = np.zeros(
    [RADAR_targetS_LEN, RADAR_targetS_LEN], dtype=np.int32)

g_display_raw_data = False
g_display_radar_model = False
g_display_eigenvalues_table = False

def base64_decode_bin(str_data):
    try:
        bin_data = base64.b64decode(str_data)
    except Exception as e:
        print(f"Exception: {e}, data: {str_data}")

    list_data = list(bin_data)

    for i in range(len(list_data)):
        if list_data[i] > 127:
            list_data[i] = list_data[i] - 256

    return list_data


def base64_encode_bin(list_data):
    for i in range(len(list_data)):
        if list_data[i] < 0:
            list_data[i] = 256 + list_data[i]
    # print(list_data)

    str_data = "test"
    try:
        str_data = base64.b64encode(bytes(list_data)).decode('utf-8')
    except Exception as e:
        print(f"Exception: {e}, data: {list_data}")
    return str_data


def get_label(folder_path):
    parts = str.split(folder_path, os.path.sep)
    return parts[-1]


def evaluate_data_send(serial_queue_write, folder_path):
    label = get_label(folder_path)
    if label == "train":
        command = f"radar --train_start"
        serial_queue_write.put(command)

    tcpCliSock = socket.socket()
    device_info_series = pd.read_csv('log/device_info.csv').iloc[-1]

    print(f"connect:{device_info_series['ip']},{device_info_series['port']}")
    tcpCliSock.connect((device_info_series['ip'], device_info_series['port']))

    file_name_list = sorted(os.listdir(folder_path))
    print(file_name_list)
    for file_name in file_name_list:
        file_path = folder_path + os.path.sep + file_name
        data_pd = pd.read_csv(file_path)
        for index, data_series in enumerate(data_pd.iloc):
            csi_raw_data = json.loads(data_series['data'])
            data_pd.loc[index, 'data'] = base64_encode_bin(csi_raw_data)
            temp_list = base64_decode_bin(data_pd.loc[index, 'data'])
            # print(f"temp_list: {temp_list}")
            data_str = ','.join(str(value)
                                for value in data_pd.loc[index]) + "\n"
            data_str = data_str.encode('utf-8')
            # print(f"data_str: {data_str}")
            tcpCliSock.send(data_str)

    tcpCliSock.close()
    time.sleep(1)

    if label == "train":
        command = "radar --train_stop"
        serial_queue_write.put(command)

    sys.exit(0)


class DataGraphicalWindow(QMainWindow, Ui_MainWindow):
    def __init__(self, serial_queue_write, parent=None):
        super(DataGraphicalWindow, self).__init__(parent)
        self.setupUi(self)
        global g_display_raw_data
        global g_display_radar_model
        global display_eigenvalues_table

        with open("./config/gui_config.json") as file:
            gui_config = json.load(file)
            # print(f"gui_config: {gui_config}")
            if len(gui_config['router_ssid']) > 0:
                self.lineEdit_router_ssid.setText(gui_config['router_ssid'])
            if len(gui_config['router_password']) >= 8:
                self.lineEdit_router_password.setText(
                    gui_config['router_password'])

            g_display_raw_data = gui_config['display_raw_data']
            if g_display_raw_data:
                self.checkBox_raw_data.setChecked(True)
            else:
                self.checkBox_raw_data.setChecked(False)

            g_display_radar_model = gui_config['display_radar_model']
            if g_display_radar_model:
                self.checkBox_radar_model.setChecked(True)
            else:
                self.checkBox_radar_model.setChecked(False)

            g_display_eigenvalues_table = gui_config['display_eigenvalues_table']
            if g_display_eigenvalues_table:
                self.checkBox_display_eigenvalues_table.setChecked(True)
            else:
                self.checkBox_display_eigenvalues_table.setChecked(False)

            if gui_config['router_auto_connect']:
                self.checkBox_router_auto_connect.setChecked(True)
            else:
                self.checkBox_router_auto_connect.setChecked(False)

        self.curve_subcarrier_range = np.array([10, 20])
        self.graphicsView_subcarrier.setYRange(
            self.curve_subcarrier_range[0], self.curve_subcarrier_range[1])
        self.graphicsView_subcarrier.addLegend()

        self.curve_subcarrier = []
        self.serial_queue_write = serial_queue_write

        for i in range(CSI_DATA_COLUMNS):
            curve = self.graphicsView_subcarrier.plot(
                g_csi_amplitude_array[:, i], name=str(i), pen=csi_vaid_subcarrier_color[i])
            self.curve_subcarrier.append(curve)

        self.curve_rssi = self.graphicsView_rssi.plot(
            g_rssi_array, name='rssi', pen=(255, 255, 255))

        self.wave_filtering_flag = self.checkBox_wave_filtering.isCheckable()
        self.checkBox_wave_filtering.released.connect(
            self.show_curve_subcarrier_filter)
        # self.graphicsView_rssi.setYRange(-10, -65)
        self.checkBox_router_auto_connect.released.connect(
            self.show_router_auto_connect)

        self.curve_radar_eigenvalue = []
        self.curve_radar_eigenvalue_threshold = []
        # self.graphicsView_eigenvalues.setYRange(0.95, 1.0)
        self.graphicsView_eigenvalues.addLegend()
        self.graphicsView_eigenvalues.showGrid(x=True, y=True)

        self.dateTimeEdit_statistics_time.setDateTime(g_current_time)
        self.statistic_config = pd.Series(index=['time', 'mode', 'auto_update'],
                                          data=[
            self.dateTimeEdit_statistics_time.dateTime().toPyDateTime(),
            self.comboBox_statistics_mode.currentText(),
            self.checkBox_statistics_auto_update.isChecked()
        ])

        for i in range(RADAR_targetS_LEN):
            curve_eigenvalue = self.graphicsView_eigenvalues.plot(
                g_radar_eigenvalue_array[:, i], name=RADAR_WAVEFORM_NAMES[i], pen=g_radar_eigenvalue_color[i])
            curve_eigenvalue_threshold = self.graphicsView_eigenvalues.plot(
                g_radar_eigenvalue_threshold_array[:, i], 
                name=RADAR_WAVEFORM_NAMES[i] + "_" + RADAR_targetS_NAMES[i] + "_threshold", 
                pen=g_radar_eigenvalue_threshold_color[i])
            self.curve_radar_eigenvalue.append(curve_eigenvalue)
            self.curve_radar_eigenvalue_threshold.append(
                curve_eigenvalue_threshold)

        self.model_radio_header = QStandardItemModel(
            1, len(g_radio_header_pd.columns.values))
        self.model_radio_header.setHorizontalHeaderLabels(
            g_radio_header_pd.columns.values)
        self.tableView_radioHeader.setModel(self.model_radio_header)
        self.tableView_radioHeader.horizontalHeader(
        ).setSectionResizeMode(QHeaderView.ResizeToContents)
        self.tableView_values = {
            'sig_mode': ['non HT(11bg)', 'HT(11n)', 'VHT(11ac)'],
            'mcs': [f'MSC{i}' for i in range(76)],
            "cwb": ['20MHz', '40MHz'],
            'aggregation': ['MPDU', 'AMPDU'],
            'channel_secondary': ['none', 'above', 'below'],
            'stbc': ['non STBC', 'STBC'],
            'sgi': ['Long GI', 'Short GI'],
        }

        self.model_device_info = QStandardItemModel(
            len(DEVICE_INFO_COLUMNS_NAMES), 2)
        self.tableView_device_info.setModel(self.model_device_info)
        self.tableView_device_info.setSizeAdjustPolicy(QAbstractScrollArea.AdjustToContents)
        self.tableView_device_info.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)

        self.model_status_record = QStandardItemModel(
            1, len(g_status_record_pd.columns.values))
        self.model_status_record.setHorizontalHeaderLabels(
            g_status_record_pd.columns.values)
        self.tableView_status_record.setModel(self.model_status_record)
        self.tableView_status_record.setSizeAdjustPolicy(QAbstractScrollArea.AdjustToContents)
        self.tableView_status_record.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)

        self.model_radar_data_room = QStandardItemModel(len(
            g_radar_data_room_pd.columns.values), len(g_radar_data_room_pd.columns.values))
        self.model_radar_data_room.setHorizontalHeaderLabels(g_radar_data_room_pd.columns.values)
        self.tableView_radar_data_room.setModel(self.model_radar_data_room)
        self.tableView_radar_data_room.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)

        self.model_radar_data_human = QStandardItemModel(len(
            g_radar_data_human_pd.columns.values), len(g_radar_data_human_pd.columns.values))
        self.model_radar_data_human.setHorizontalHeaderLabels(
            g_radar_data_human_pd.columns.values)
        self.tableView_radar_data_human.setModel(self.model_radar_data_human)
        self.tableView_radar_data_human.horizontalHeader(
        ).setSectionResizeMode(QHeaderView.ResizeToContents)

        evaluate_STATISTICS_COLUMNS_NAMES = ["type", "count", "rate"]
        self.model_evaluate_statistics = QStandardItemModel(
            len(evaluate_STATISTICS_COLUMNS_NAMES), 3)
        self.model_evaluate_statistics.setHorizontalHeaderLabels(
            evaluate_STATISTICS_COLUMNS_NAMES)
        self.model_evaluate_statistics.setItem(
            0, 0, QStandardItem('none static'))
        self.model_evaluate_statistics.setItem(1, 0, QStandardItem('none move'))
        self.model_evaluate_statistics.setItem(
            2, 0, QStandardItem('someone static'))
        self.model_evaluate_statistics.setItem(
            3, 0, QStandardItem('someone move'))

        self.pushButton_predict_config.released.connect(self.command_predict_config)
        self.pushButton_statistics_config.released.connect(
            self.show_statistics_status_record_move)
        self.comboBox_statistics_mode.activated.connect(
            self.show_statistics_status_record_move)
        # self.pushButton_evaluate_config.released.connect(self.command_evaluate_config)

        self.pushButton_router_connect.released.connect(
            self.command_router_connect)
        self.pushButton_command.released.connect(self.command_custom)

        self.comboBox_command.activated.connect(self.comboBox_command_show)

        self.pushButton_collect_start.released.connect(
            self.pushButton_collect_show)
        self.pushButton_train_start.released.connect(
            self.pushButton_train_show)

        self.pushButton_collect_clean.released.connect(
            self.pushButton_collect_clean_show)

        self.checkBox_raw_data.released.connect(self.checkBox_raw_data_show)
        self.checkBox_radar_model.released.connect(self.checkBox_radar_model_show)
        self.checkBox_display_eigenvalues_table.released.connect(
            self.checkBox_display_eigenvalues_table_show)

        self.splitter_eigenvalues.setStretchFactor(0, 8)
        self.splitter_eigenvalues.setStretchFactor(1, 1)

        self.splitter_status_record.setStretchFactor(0, 10)
        self.splitter_status_record.setStretchFactor(1, 1)

        self.splitter_raw_data.setStretchFactor(0, 1)
        self.splitter_raw_data.setStretchFactor(1, 8)
        self.splitter_raw_data.setStretchFactor(2, 2)

        self.textBrowser_log.setStyleSheet("background:black")

        self.timer_boot_command = QTimer()
        self.timer_boot_command.timeout.connect(self.command_boot)
        self.timer_boot_command.setInterval(3000)
        self.timer_boot_command.start()

        self.timer_curve_subcarrier = QTimer()
        self.timer_curve_subcarrier.timeout.connect(self.show_curve_subcarrier)
        self.timer_curve_subcarrier.setInterval(300)

        self.timer_curve_radar = QTimer()
        self.timer_curve_radar.timeout.connect(self.show_curve_eigenvalue)
        self.timer_curve_radar.setInterval(100)

        self.timer_eigenvalues_table = QTimer()
        self.timer_eigenvalues_table.timeout.connect(
            self.show_eigenvalue_table)
        self.timer_eigenvalues_table.setInterval(100)

        self.label_number = self.spinBox_collect_number.value()
        self.timer_collect_duration = QTimer()
        self.timer_collect_duration.timeout.connect(
            self.spinBox_collect_number_show)

        self.label_delay = self.timeEdit_collect_delay.time()
        self.timer_collect_delay = QTimer()
        self.timer_collect_delay.setInterval(1000)
        self.timer_collect_delay.timeout.connect(self.timeEdit_collect_delay_show)

        self.train_duration = self.timeEdit_train_duration.time()
        self.timer_train_duration = QTimer()
        self.timer_train_duration.setInterval(1000)
        self.timer_train_duration.timeout.connect(
            self.spinBox_train_duration_show)

        self.train_delay = self.timeEdit_train_delay.time()
        self.timer_train_delay = QTimer()
        self.timer_train_delay.setInterval(1000)
        self.timer_train_delay.timeout.connect(self.timeEdit_train_delay_show)

        self.timer_status_record = QTimer()
        self.timer_status_record.timeout.connect(self.show_statistics_status_record)
        self.timer_status_record.setInterval(1000)

        self.timer_evaluate_statistics = QTimer()
        self.timer_evaluate_statistics.timeout.connect(self.show_evaluate_statistics)
        self.timer_evaluate_statistics.setInterval(1000)

        self.checkBox_raw_data_show()
        self.checkBox_radar_model_show()
        self.checkBox_display_eigenvalues_table_show()

    def checkBox_raw_data_show(self):
        global g_display_raw_data
        g_display_raw_data = self.checkBox_raw_data.isChecked()

        if self.checkBox_raw_data.isChecked():
            self.groupBox_raw_data.show()
            self.timer_curve_subcarrier.start()
        else:
            self.groupBox_raw_data.hide()
            self.timer_curve_subcarrier.stop()

        with open("./config/gui_config.json", "r") as file:
            gui_config = json.load(file)
            gui_config['display_raw_data'] = self.checkBox_raw_data.isChecked()
        with open("./config/gui_config.json", "w") as file:
            json.dump(gui_config, file)

    def checkBox_radar_model_show(self):
        global g_display_radar_model

        g_display_radar_model = self.checkBox_radar_model.isChecked()

        if self.checkBox_radar_model.isChecked():
            self.groupBox_radar_model.show()
            self.timer_status_record.start()
            self.timer_curve_radar.start()
        else:
            self.groupBox_radar_model.hide()
            self.timer_status_record.stop()
            self.timer_curve_radar.stop()

        with open("./config/gui_config.json", "r") as file:
            gui_config = json.load(file)
            gui_config['display_radar_model'] = self.checkBox_radar_model.isChecked()
        with open("./config/gui_config.json", "w") as file:
            json.dump(gui_config, file)

    def checkBox_display_eigenvalues_table_show(self):
        global g_display_eigenvalues_table
        g_display_eigenvalues_table = self.checkBox_display_eigenvalues_table.isChecked()

        if self.checkBox_display_eigenvalues_table.isChecked():
            self.tableView_eigenvalues.show()
            self.timer_eigenvalues_table.start()
        else:
            self.tableView_eigenvalues.hide()
            self.timer_eigenvalues_table.stop()

        with open("./config/gui_config.json", "r") as file:
            gui_config = json.load(file)
            gui_config['display_eigenvalues_table'] = self.checkBox_display_eigenvalues_table.isChecked()
        with open("./config/gui_config.json", "w") as file:
            json.dump(gui_config, file)

    def show_router_auto_connect(self):
        with open("./config/gui_config.json", "r") as file:
            gui_config = json.load(file)
            gui_config['router_auto_connect'] = self.checkBox_router_auto_connect.isChecked()
        with open("./config/gui_config.json", "w") as file:
            json.dump(gui_config, file)

    def median_filtering(self, waveform):
        tmp = waveform
        for i in range(1, waveform.shape[0] - 1):
            outliers_count = 0
            for j in range(waveform.shape[1]):
                if ((waveform[i - 1, j] - waveform[i, j] > 2 and waveform[i + 1, j] - waveform[i, j] > 2) 
                or (waveform[i - 1, j] - waveform[i, j] < -2 and waveform[i + 1, j] - waveform[i, j] < -2)):
                    outliers_count += 1
                    continue

            if outliers_count > 16:
                for x in range(1, waveform.shape[1] - 1):
                    tmp[i, x] = (waveform[i - 1, x] + waveform[i + 1, x]) / 2
        waveform = tmp

    def show_curve_subcarrier(self):
        wn = 20 / (CSI_SAMPLE_RATE / 2)
        b, a = signal.butter(8, wn, 'lowpass')

        if self.wave_filtering_flag:
            self.median_filtering(g_csi_amplitude_array)
            csi_filtfilt_data = signal.filtfilt(b, a, g_csi_amplitude_array.T).T
        else:
            csi_filtfilt_data = g_csi_amplitude_array

        if self.curve_subcarrier_range[0] > csi_filtfilt_data.min() or self.curve_subcarrier_range[1] < csi_filtfilt_data.max():
            if csi_filtfilt_data.min() > 0 and self.curve_subcarrier_range[0] > csi_filtfilt_data.min():
                self.curve_subcarrier_range[0] = csi_filtfilt_data.min() - 1
            if self.curve_subcarrier_range[1] < csi_filtfilt_data.max():
                self.curve_subcarrier_range[1] = csi_filtfilt_data.max() + 1
            self.graphicsView_subcarrier.setYRange(
                self.curve_subcarrier_range[0], self.curve_subcarrier_range[1])

        for i in range(CSI_DATA_COLUMNS):
            self.curve_subcarrier[i].setData(csi_filtfilt_data[:, i])

        if self.wave_filtering_flag:
            csi_filtfilt_rssi = signal.filtfilt(
                b, a, g_rssi_array).astype(np.int32)
        else:
            csi_filtfilt_rssi = g_rssi_array
        self.curve_rssi.setData(csi_filtfilt_rssi)

        for i in range(g_radio_header_pd.shape[0]):
            for j in range(g_radio_header_pd.shape[1]):
                if g_radio_header_pd.columns.values[j] in self.tableView_values.keys():
                    str_values = self.tableView_values[g_radio_header_pd.columns.values[j]][int(
                        g_radio_header_pd.iloc[i, j])]
                    item = QStandardItem(str_values)
                else:
                    # print(j, g_radio_header_pd.columns.values[j])
                    item = QStandardItem(g_radio_header_pd.iloc[i, j])
                self.model_radio_header.setItem(i, j, item)

    def show_curve_subcarrier_filter(self):
        self.wave_filtering_flag = self.checkBox_wave_filtering.isChecked()

    def show_curve_eigenvalue(self):
        for i in range(RADAR_targetS_LEN):
            self.curve_radar_eigenvalue[i].setData(
                g_radar_eigenvalue_array[:, i])
            self.curve_radar_eigenvalue_threshold[i].setData(
                g_radar_eigenvalue_threshold_array[:, i])

        curve_radar_title = f"{ROOM_STATUS_NAMES[g_radar_status_array[-1,0]]} {HUMAN_STATUS_NAMES[g_radar_status_array[-1,1]]}"
        self.graphicsView_eigenvalues.setTitle(curve_radar_title)

    def show_eigenvalue_table(self):
        for i in range(g_radar_data_room_pd.shape[0]):
            for j in range(g_radar_data_room_pd.shape[1]):
                # data_str = "%.5f" % 0.01
                # print(f"{g_radar_data_room_pd.iloc[i,j]}, {type(g_radar_data_room_pd.iloc[i,j])}")
                data_str = "%.5f" % g_radar_data_room_pd.iloc[i, j]
                item = QStandardItem(data_str)
                self.model_radar_data_room.setItem(i, j, item)

        for i in range(g_radar_data_human_pd.shape[0]):
            for j in range(g_radar_data_human_pd.shape[1]):
                # print(f"{g_radar_data_human_pd.iloc[i,j]}, {type(g_radar_data_human_pd.iloc[i,j])}")
                data_str = "%.5f" % g_radar_data_human_pd.iloc[i, j]
                item = QStandardItem(data_str)
                self.model_radar_data_human.setItem(i, j, item)

    def show_device_info(self, device_info_series):
        # print(device_info_series)
        for i in range(len(device_info_series)):
            self.model_device_info.setItem(
                i, 0, QStandardItem(device_info_series.index[i]))
            self.model_device_info.setItem(
                i, 1, QStandardItem(str(device_info_series.values[i])))
        # self.tableView_device_info.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)

    def get_statistic_config(self):
        self.statistic_config['time'] = self.dateTimeEdit_statistics_time.dateTime().toPyDateTime()
        self.statistic_config['mode'] = self.comboBox_statistics_mode.currentText()
        self.statistic_config['auto_update'] = self.checkBox_statistics_auto_update.isChecked()

    def show_statistics_status_record_move(self):
        self.get_statistic_config()

        if self.statistic_config['mode'] == 'day':
            statistic_move_array = np.zeros(24, dtype=np.int32)
            statistic_move_title = datetime.strftime(
                self.statistic_config['time'], "%Y-%m-%d")
            statistic_move_pd = g_move_record_pd[g_move_record_pd['date']
                                                 == self.statistic_config['time'].date()]

            if len(statistic_move_pd):
                statistic_move_hour = statistic_move_pd.groupby(by='hour')[
                    'count'].sum()

                for index, value in statistic_move_hour.items():
                    statistic_move_array[index] = value

        elif self.statistic_config['mode'] == 'hour':
            statistic_move_array = np.zeros(60, dtype=np.int32)
            statistic_move_title = datetime.strftime(
                self.statistic_config['time'], "%Y-%m-%d %H")

            statistic_move_pd = g_move_record_pd[(g_move_record_pd['date'] == self.statistic_config['time'].date()) & (
                g_move_record_pd['hour'] == self.statistic_config['time'].time().hour)]

            if len(statistic_move_pd):
                statistic_move_minute = statistic_move_pd.groupby(by='minute')[
                    'count'].sum()
                for index, value in statistic_move_minute.items():
                    statistic_move_array[index] = value

        elif self.statistic_config['mode'] == 'minute':
            statistic_move_array = np.zeros(60, dtype=np.int32)
            statistic_move_title = datetime.strftime(
                self.statistic_config['time'], "%Y-%m-%d %H:%M")

            statistic_move_pd = g_move_record_pd[(g_move_record_pd['date'] == self.statistic_config['time'].date())
                                                 & (g_move_record_pd['hour'] == self.statistic_config['time'].time().hour)
                                                      & (g_move_record_pd['hour'] == self.statistic_config['time'].time().hour) 
                                                 & (g_move_record_pd['hour'] == self.statistic_config['time'].time().hour)
                                                 & (g_move_record_pd['minute'] == self.statistic_config['time'].time().minute)]

            if len(statistic_move_pd):
                statistic_move_second = statistic_move_pd.groupby(by='second')[
                    'count'].sum()
                for index, value in statistic_move_second.items():
                    statistic_move_array[index] = value
        else:
            print(f"fail mode: {self.statistic_config['mode']}")

        try:
            self.graphicsView_status_record.removeItem(self.bg)
        except Exception as e:
            # print(e)
            pass

        try:
            self.bg = pq.BarGraphItem(x=range(
                len(statistic_move_array)), height=statistic_move_array, width=0.3, brush='g')
            self.graphicsView_status_record.addItem(self.bg)

            self.graphicsView_status_record.showGrid(x=True, y=True)
            self.graphicsView_status_record.setXRange(
                0, len(statistic_move_array))
            # print(statistic_move_title)
            self.graphicsView_status_record.setTitle(
                statistic_move_title + "  Move Count")
        except Exception as e:
            print(e)

    def show_statistics_status_record(self):
        if self.checkBox_statistics_auto_update.isChecked():
            self.dateTimeEdit_statistics_time.setDateTime(g_current_time)
            self.show_statistics_status_record_move()

        # print(f"g_status_record table: {g_status_record_pd.shape}")

        for i in range(g_status_record_pd.shape[0]):
            for j in range(g_status_record_pd.shape[1]):
                item = QStandardItem(g_status_record_pd.iloc[i, j])
                self.model_status_record.setItem(i, j, item)

    def show_evaluate_statistics(self):
        if not g_evaluate_statistics_array.sum():
            return

        for i in range(g_evaluate_statistics_array.shape[0]):
            for j in range(g_evaluate_statistics_array.shape[1]):
                row = i * 2 + j

                item = QStandardItem(str(g_evaluate_statistics_array[i][j]))
                self.model_evaluate_statistics.setItem(row, 1, item)

                data_str = "%.2f%%" % (
                    g_evaluate_statistics_array[i][j] * 100 / g_evaluate_statistics_array.sum())
                item = QStandardItem(data_str)
                self.model_evaluate_statistics.setItem(row, 2, item)

    def command_boot(self):
        command = f"radar --csi_output_type LLFT --csi_output_format base64"
        self.serial_queue_write.put(command)

        if self.checkBox_router_auto_connect.isChecked() and len(self.lineEdit_router_ssid.text()) > 0:
            self.command_router_connect()

        self.timer_boot_command.stop()

    def command_predict_config(self):
        command = (f"radar --predict_someone_sensitivity {self.doubleSpinBox_predict_someone_sensitivity.value()}" +
                   f" --predict_move_sensitivity {self.doubleSpinBox_predict_move_sensitivity.value()}")
        self.serial_queue_write.put(command)
        command = (f"radar --predict_buff_size {self.spinBox_predict_buffer_size.text()}" +
                   f" --predict_outliers_number {self.spinBox_predict_outliers_number.text()}")
        self.serial_queue_write.put(command)

    def command_router_connect(self):
        if self.pushButton_router_connect.text() == "connect":
            self.pushButton_router_connect.setText("disconnect")
            self.pushButton_router_connect.setStyleSheet("color: red")

            command = "wifi_config --ssid " + ("\"%s\"" % self.lineEdit_router_ssid.text())
            if len(self.lineEdit_router_password.text()) >= 8:
                command += " --password " + self.lineEdit_router_password.text()
            self.serial_queue_write.put(command)
        else:
            self.pushButton_router_connect.setText("connect")
            self.pushButton_router_connect.setStyleSheet("color: black")
            command = "ping --abort"
            self.serial_queue_write.put(command)
            command = "wifi_config --disconnect"
            self.serial_queue_write.put(command)

        with open("./config/gui_config.json", "r") as file:
            gui_config = json.load(file)
            gui_config['router_ssid'] = self.lineEdit_router_ssid.text()
            gui_config['router_password'] = self.lineEdit_router_password.text()
        with open("./config/gui_config.json", "w") as file:
            json.dump(gui_config, file)

    def command_custom(self):
        command = self.lineEdit_command.text()
        self.serial_queue_write.put(command)

    def command_collect_target_start(self):
        command = (f"radar --collect_number {self.spinBox_collect_number.value()}" +
                   f" --collect_tagets {self.comboBox_collect_target.currentText()}" +
                   f" --collect_duration {self.spinBox_collect_duration.value()}")
        self.serial_queue_write.put(command)

    def command_collect_target_stop(self):
        command = "radar --collect_number 0 --collect_tagets unknown"
        self.serial_queue_write.put(command)

    def spinBox_collect_number_show(self):
        number = self.spinBox_collect_number.value()
        if number >= 1:
            self.spinBox_collect_number.setValue(number - 1)
            self.timer_collect_duration.start()
            # self.command_collect_target_start()
        else:
            self.timer_collect_duration.stop()
            # self.command_collect_target_stop()
            self.spinBox_collect_number.setValue(self.label_number)
            self.timeEdit_collect_delay.setTime(self.label_delay)
            self.pushButton_collect_start.setStyleSheet("color: black")
            self.spinBox_collect_number.setStyleSheet("color: black")
            self.pushButton_collect_start.setText("start")

    def timeEdit_collect_delay_show(self):
        time_temp = self.timeEdit_collect_delay.time()
        second = time_temp.hour() * 3600 + time_temp.minute() * 60 + time_temp.second()
        if second > 0:
            self.timeEdit_collect_delay.setTime(time_temp.addSecs(-1))
            self.timer_collect_delay.start()
        else:
            self.timer_collect_delay.stop()

            duration = self.spinBox_collect_duration.value()
            self.timer_collect_duration.setInterval(duration)
            self.timer_collect_duration.start()
            self.command_collect_target_start()
            self.spinBox_collect_number.setStyleSheet("color: red")
            self.timeEdit_collect_delay.setStyleSheet("color: black")

    def pushButton_collect_show(self):
        if self.pushButton_collect_start.text() == "start":
            if self.comboBox_collect_target.currentIndex() == 0 or self.spinBox_collect_number.value() == 0:
                err = QErrorMessage(self)
                err.setWindowTitle('Label parameter error')
                err.showMessage(
                    "Please check whether 'taget' or 'number' is set")
                err.show()
                return

            self.pushButton_collect_start.setText("stop")
            self.pushButton_collect_start.setStyleSheet("color: red")
            self.timeEdit_collect_delay.setStyleSheet("color: red")
            self.timer_collect_delay.start()
            self.label_number = self.spinBox_collect_number.value()
            self.label_delay = self.timeEdit_collect_delay.time()
        else:
            self.spinBox_collect_number.setValue(self.label_number)
            self.timeEdit_collect_delay.setTime(self.label_delay)
            self.spinBox_collect_number.setStyleSheet("color: black")
            self.pushButton_collect_start.setStyleSheet("color: black")
            self.timer_collect_delay.stop()
            self.timer_collect_duration.stop()
            self.command_collect_target_stop()
            self.pushButton_collect_start.setText("start")

    def pushButton_collect_clean_show(self):
        folder_path = 'data'

        message = QMessageBox.warning(
            self, 'Warning', "will delete all files in 'data'", QMessageBox.Ok | QMessageBox.Cancel, QMessageBox.Ok)
        if message == QMessageBox.Cancel:
            return

        for root, dirs, files in os.walk(folder_path, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))

    def command_train_start(self):
        command = f"radar --train_start"

        if self.checkBox_train_add.isChecked():
            command += " --train_add"

        self.serial_queue_write.put(command)

    def command_train_stop(self):
        command = "radar --train_stop"
        self.serial_queue_write.put(command)

    def spinBox_train_duration_show(self):
        time_temp = self.timeEdit_train_duration.time()
        second = time_temp.hour() * 3600 + time_temp.minute() * 60 + time_temp.second()
        if second > 0:
            self.timeEdit_train_duration.setTime(time_temp.addSecs(-1))
            self.timer_train_duration.start()
        else:
            self.timer_train_duration.stop()
            self.command_train_stop()
            self.timeEdit_train_delay.setTime(self.train_delay)
            self.timeEdit_train_duration.setTime(self.train_duration)
            self.timeEdit_train_duration.setStyleSheet("color: black")
            self.pushButton_train_start.setText("start")
            self.pushButton_train_start.setStyleSheet("color: black")

    def timeEdit_train_delay_show(self):
        time_temp = self.timeEdit_train_delay.time()
        second = time_temp.hour() * 3600 + time_temp.minute() * 60 + time_temp.second()
        if second > 0:
            self.timeEdit_train_delay.setTime(time_temp.addSecs(-1))
            self.timer_train_delay.start()
        else:
            self.timer_train_delay.stop()
            self.command_train_start()
            self.spinBox_train_duration_show()
            self.timeEdit_train_duration.setStyleSheet("color: red")
            self.timeEdit_train_delay.setStyleSheet("color: black")

    def pushButton_train_show(self):
        if self.pushButton_train_start.text() == "start":
            reply = QMessageBox.question(self, 'Note', 'During environmental calibration, it must be ensured that no one is in the room',
                                         QMessageBox.Yes | QMessageBox.Cancel, QMessageBox.Yes)
            if reply == QMessageBox.Cancel:
                return

            self.train_delay = self.timeEdit_train_delay.time()
            self.train_duration = self.timeEdit_train_duration.time()
            self.pushButton_train_start.setText("stop")
            self.pushButton_train_start.setStyleSheet("color: red")
            self.timeEdit_train_delay.setStyleSheet("color: red")

            self.timeEdit_train_delay_show()
        else:
            self.timer_train_delay.stop()
            self.timer_train_duration.stop()
            self.command_train_stop()
            self.timeEdit_train_delay.setTime(self.train_delay)
            self.timeEdit_train_duration.setTime(self.train_duration)
            self.pushButton_train_start.setText("start")
            self.pushButton_train_start.setStyleSheet("color: black")

    def comboBox_command_show(self):
        self.lineEdit_command.setText(self.comboBox_command.currentText())

    def show_textBrowser_log(self, str):
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
    # Rotate data
    g_csi_amplitude_array[:-1] = g_csi_amplitude_array[1:]
    g_rssi_array[:-1] = g_rssi_array[1:]
    g_radio_header_pd.iloc[1:] = g_radio_header_pd.iloc[:-1]

    csi_raw_data = data['data']
    for i in range(CSI_DATA_COLUMNS):
        data_complex = complex(csi_raw_data[csi_vaid_subcarrier_index[i] * 2],
                               csi_raw_data[csi_vaid_subcarrier_index[i] * 2 - 1])
        g_csi_amplitude_array[-1][i] = np.abs(data_complex)

    g_rssi_array[-1] = data['rssi']
    g_radio_header_pd.loc[0] = data[1:len(CSI_DATA_COLUMNS_NAMES)-1]
    # print(g_radio_header_pd.loc[0])

    return


def radar_data_handle(self, data):
    g_radar_eigenvalue_array[:-1] = g_radar_eigenvalue_array[1:]
    g_radar_eigenvalue_threshold_array[:-
                                       1] = g_radar_eigenvalue_threshold_array[1:]
    g_radar_status_array[:-1] = g_radar_status_array[1:]

    global g_current_time

    current_time_str = data['timestamp']
    g_current_time = datetime.strptime(
        data['timestamp'], '%Y-%m-%d %H:%M:%S.%f')

    for taget_index in range(RADAR_targetS_LEN):
        g_radar_eigenvalue_array[-1][taget_index] = data[f"waveform_{RADAR_WAVEFORM_NAMES[taget_index]}"]
        g_radar_eigenvalue_threshold_array[-1][taget_index] = data[
            f"waveform_{RADAR_WAVEFORM_NAMES[taget_index]}_threshold"]
        g_radar_status_array[-1][taget_index] = data[f"{RADAR_targetS_NAMES[taget_index]}_status"]

    if g_radar_status_array[-1][0] != g_radar_status_array[-2][0] or g_radar_status_array[-1][1] != g_radar_status_array[-2][1]:
        g_status_record_pd[1:] = g_status_record_pd[:-1]

        g_status_record_pd.loc[0, 'start_time'] = current_time_str
        g_status_record_pd.loc[0, 'room'] = ROOM_STATUS_NAMES[g_radar_status_array[-1][0]]
        g_status_record_pd.loc[0, 'human'] = HUMAN_STATUS_NAMES[g_radar_status_array[-1][1]]

        if len(g_status_record_pd.loc[1, 'start_time']):
            g_status_record_pd.loc[1, 'stop_time'] = current_time_str

    if len(g_status_record_pd.loc[0, 'start_time']):
        temp_time = g_current_time - \
            datetime.strptime(
                g_status_record_pd.loc[0, 'start_time'], '%Y-%m-%d %H:%M:%S.%f')
        g_status_record_pd.loc[0, 'spend_time'] = f"{temp_time}"[:-3]

    if g_radar_status_array[-1][1]:
        index = len(g_move_record_pd)
        g_move_record_pd.loc[index] = [g_current_time.date(), g_current_time.time(
        ).hour, g_current_time.time().minute, g_current_time.time().second, 1]

    g_evaluate_statistics_array[g_radar_status_array[-1,
                                                   0], g_radar_status_array[-1, 1]] += 1
    if (g_radar_eigenvalue_threshold_array[-1, 0] != g_radar_eigenvalue_threshold_array[-2, 0] 
        or g_radar_eigenvalue_threshold_array[-1, 1] != g_radar_eigenvalue_threshold_array[-2, 1]):
        self.signal_wareform_threshold.emit()

    if g_display_eigenvalues_table:
        try:
            g_radar_data_room_pd[1:] = g_radar_data_room_pd[:-1]
            g_radar_data_room_pd.loc[0]['status'] = g_radar_status_array[-1, 0]
            g_radar_data_room_pd.loc[0]['threshold'] = g_radar_eigenvalue_threshold_array[-1, 0]
            g_radar_data_room_pd.loc[0]['value'] = g_radar_eigenvalue_array[-1, 0]

            value_buff = g_radar_eigenvalue_array[-10:, 0]
            g_radar_data_room_pd.loc[0]['max'] = np.max(value_buff)
            g_radar_data_room_pd.loc[0]['min'] = np.min(value_buff)
            g_radar_data_room_pd.loc[0]['mean'] = np.mean(value_buff)
            g_radar_data_room_pd.loc[0]['std'] = np.std(value_buff)

            g_radar_data_human_pd[1:] = g_radar_data_human_pd[:-1]
            g_radar_data_human_pd.loc[0]['status'] = g_radar_status_array[-1, 1]
            g_radar_data_human_pd.loc[0]['threshold'] = g_radar_eigenvalue_threshold_array[-1, 1]
            g_radar_data_human_pd.loc[0]['value'] = g_radar_eigenvalue_array[-1, 1]

            value_buff = g_radar_eigenvalue_array[-10:, 1]
            g_radar_data_human_pd.loc[0]['max'] = np.max(value_buff)
            g_radar_data_human_pd.loc[0]['min'] = np.min(value_buff)
            g_radar_data_human_pd.loc[0]['mean'] = np.mean(value_buff)
            g_radar_data_human_pd.loc[0]['std'] = np.std(value_buff)
        except Exception as e:
            print(e)
    return

class DataHandleThread(QThread):
    signal_log_msg = pyqtSignal(str)
    signal_radar_time_domain = pyqtSignal(str)
    signal_exit = pyqtSignal()
    signal_device_info = pyqtSignal(pd.Series)
    signal_wareform_threshold = pyqtSignal()

    def __init__(self, serial_queue_read):
        super().__init__()

        self.serial_queue_read = serial_queue_read
        self.taget_count = 0

    def run(self):
        while True:
            # print(f"g_display_raw_data: {g_display_raw_data}")
            series = self.serial_queue_read.get()
            if series['type'] == 'DEVICE_INFO' and g_display_raw_data:
                g_device_info_series = series.copy()
                self.signal_device_info.emit(series)
            elif series['type'] == 'CSI_DATA' and g_display_raw_data:
                csi_data_handle(self, series)
            elif series['type'] == 'RADAR_DADA' and g_display_radar_model:
                radar_data_handle(self, series)
            elif series['type'] == 'LOG_DATA' and g_display_raw_data:
                if series['tag'] == 'I':
                    color = "green"
                elif series['tag'] == 'W':
                    color = "yellow"
                elif series['tag'] == 'E':
                    color = "red"
                else:
                    color = "white"

                data = "<font color=\'%s\'>%s (%s) %s <font>" % (
                    color, series['tag'], series['timestamp'], series['data'])
                self.signal_log_msg.emit(data)
            elif series['type'] == 'FAIL_EVENT':
                print(f"Fial: {series['data']}")

                self.signal_exit.emit()
                time.sleep(1)
                sys.exit(0)
            else:
                # print("error: ", series)
                pass

            QApplication.processEvents()

def serial_handle(queue_read, queue_write, port):
    try:
        ser = serial.Serial(port=port, baudrate=2000000,
                            bytesize=8, parity='N', stopbits=1, timeout=0.1)
    except Exception as e:
        print(f"serial_handle: {e}")
        data_series = pd.Series(index=['type', 'data'],
                                data=['FAIL_EVENT', "Failed to open serial port"])
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

    data_valid_list = pd.DataFrame(columns=['type', 'columns_names', 'file_name', 'file_fd', 'file_writer'],
                                   data=[["CSI_DATA", CSI_DATA_COLUMNS_NAMES, "log/csi_data.csv", None, None],
                                         ["RADAR_DADA", RADAR_DATA_COLUMNS_NAMES, "log/radar_data.csv", None, None],
                                         ["DEVICE_INFO", DEVICE_INFO_COLUMNS_NAMES, "log/device_info.csv", None, None]])

    for data_valid in data_valid_list.iloc:
        # print(type(data_valid), data_valid)
        # print(f"file_name: {data_valid['file_name']}")
        data_valid['file_fd'] = open(data_valid['file_name'], 'w')
        data_valid['file_writer'] = csv.writer(data_valid['file_fd'])
        data_valid['file_writer'].writerow(data_valid['columns_names'])

    log_data_writer = open("log/log_data.txt", 'w+')
    taget_last = 'unknown'
    taget_seq_last = 0

    ser.write("restart\r\n".encode('utf-8'))
    time.sleep(0.01)

    while True:
        if not queue_write.empty():
            command = queue_write.get()

            if command == "exit":
                sys.exit()
                break

            command = command + "\r\n"
            ser.write(command.encode('utf-8'))
            print(f"{datetime.now()}, serial write: {command}")
            continue

        try:
            strings = str(ser.readline())
            if not strings:
                continue
        except Exception as e:
            data_series = pd.Series(index=['type', 'data'],
                                    data=['FAIL_EVENT', "Failed to read serial"])
            queue_read.put(data_series)
            sys.exit()

        strings = strings.lstrip('b\'').rstrip('\\r\\n\'')
        if not strings:
            continue

        # print(f"fail: {strings}")
        for data_valid in data_valid_list.iloc:
            index = strings.find(data_valid['type'])
            if index >= 0:
                strings = strings[index:]
                csv_reader = csv.reader(StringIO(strings))
                data = next(csv_reader)

                if len(data) == len(data_valid['columns_names']):
                    data_series = pd.Series(
                        data, index=data_valid['columns_names'])

                    try:
                        datetime.strptime(
                            data_series['timestamp'], '%Y-%m-%d %H:%M:%S.%f')
                    except Exception as e:
                        data_series['timestamp'] = datetime.now().strftime(
                            '%Y-%m-%d %H:%M:%S.%f')[:-3]
                        # print(e)

                    # print(data_series)
                    if data_series['type'] == 'CSI_DATA':
                        try:
                            # csi_raw_data = json.loads(data_series['data'])
                            csi_raw_data = base64_decode_bin(
                                data_series['data'])
                            if len(csi_raw_data) != int(data_series['len']):
                                # if len(csi_raw_data) != 104 and len(csi_raw_data) != 216 and len(csi_raw_data) != 328 and len(csi_raw_data) != 552:
                                print(
                                    f"CSI_DATA, expected: {data_series['len']}, actual: {len(csi_raw_data)}")
                                break
                        except Exception as e:
                            print(
                                f"json.JSONDecodeError: {e}, data: {data_series['data']}")
                            break

                        data_series['data'] = csi_raw_data

                        if data_series['taget'] != 'unknown':
                            if data_series['taget'] != taget_last or data_series['taget_seq'] != taget_seq_last:
                                folder = f"data/{data_series['taget']}"
                                if not path.exists(folder):
                                    mkdir(folder)

                                csi_target_data_file_name = f"{folder}/{datetime.now().strftime('%Y-%m-%d_%H-%M-%S-%f')[:-3]}_{data_series['len']}_{data_series['taget_seq']}.csv"
                                print(csi_target_data_file_name)
                                csi_target_data_file_fd = open(
                                    csi_target_data_file_name, 'w+')
                                taget_data_writer = csv.writer(
                                    csi_target_data_file_fd)
                                taget_data_writer.writerow(data_series.index)

                            taget_data_writer.writerow(
                                data_series.astype(str))

                        taget_last = data_series['taget']
                        taget_seq_last = data_series['taget_seq']

                        if queue_read.full():
                            # print('============== queue_full ==========')
                            pass
                        else:
                            # print("data_series", len(data_series), type(data_series), data_series)
                            queue_read.put(data_series)
                    else:
                        queue_read.put(data_series)

                    data_valid['file_writer'].writerow(data_series.astype(str))
                    data_valid['file_fd'].flush()
                    break
        else:
            strings = re.sub(r'\\x1b.*?m', '', strings)
            log_data_writer.writelines(strings + "\n")

            log = re.match(r'.*([DIWE]) \((\d+)\) (.*)', strings, re.I)

            if not log:
                continue

            data_series = pd.Series(index=['type', 'tag', 'timestamp', 'data'],
                                    data=['LOG_DATA', log.group(1), log.group(2), log.group(3)])
            if not queue_read.full():
                queue_read.put(data_series)


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

    serial_queue_read = Queue(maxsize=64)
    serial_queue_write = Queue(maxsize=64)

    signal_key.signal(signal_key.SIGINT, quit)
    signal_key.signal(signal_key.SIGTERM, quit)

    serial_handle_process = Process(target=serial_handle, args=(
        serial_queue_read, serial_queue_write, serial_port))
    serial_handle_process.start()

    app = QApplication(sys.argv)
    app.setWindowIcon(QIcon('../../../docs/_static/icon.png'))

    window = DataGraphicalWindow(serial_queue_write)
    data_handle_thread = DataHandleThread(serial_queue_read)
    data_handle_thread.signal_device_info.connect(window.show_device_info)
    data_handle_thread.signal_log_msg.connect(window.show_textBrowser_log)
    data_handle_thread.signal_exit.connect(window.close)
    data_handle_thread.start()

    window.show()
    sys.exit(app.exec())
    serial_handle_process.join()
