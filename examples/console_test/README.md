# CSI Test Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

## Introduction

You can control/interact with the interactive shell running on the ESP32 through the serial port (UART) to test the CSI. For the usage of creating an interactive shell on the ESP32, see: [Console Component](https://docs.espressif. com/projects/esp-idf/en/latest/api-guides/console.html#console)

1. Different CSI sources: test routers, devices, and package sending devices to obtain CSI information
2. Output CSI raw data
3. Configuration for human movement detection

## How to use examples
### Hardware Required
This example should be able to run on any commonly used ESP32 development board. At least two development boards should be prepared, and the two development boards should be placed perpendicular to the ground during testing. 
If you don’t have two devices, you can use the router to act as device A
### Demo steps to test
1. Burn the firmware to two ESP32 devices separately
2. Configure a device A in ESP32 SoftAp mode
```shell
csi> ap csi_softap
I (6389) wifi:mode : softAP (30:ae:a4:00:4b:91)
I (6389) wifi:Total power save buffer number: 16
I (6389) wifi:Init max length of beacon: 752/752
I (6389) wifi:Init max length of beacon: 752/752
I (6399) wifi:Total power save buffer number: 16
I (6409) wifi_cmd: Starting SoftAP SSID: csi_softap, Password: 
I (8159) wifi:new:<13,2>, old:<13,2>, ap:<13,2>, sta:<255,255>, prof:13
I (8159) wifi:station: 30:ae:a4:80:5c:cc join, AID=1, bgn, 40D
```

2. Configure device B to ESP32 Sta mode
```shell
csi> sta csi_softap
I (67854) wifi_cmd: sta connecting to 'csi_softap'
I (67854) wifi:mode : sta (30:ae:a4:80:5c:cc)
I (67854) wifi:enable tsf
I (69624) wifi:new:<13,2>, old:<6,1>, ap:<255,255>, sta:<13,2>, prof:6
I (69624) wifi:state: init -> auth (b0)
I (69644) wifi:state: auth -> assoc (0)
I (69654) wifi:state: assoc -> run (10)
I (69664) wifi:connected with csi_softap, aid = 1, channel 13, 40D, bssid = 30:ae:a4:00:4b:91
I (69664) wifi:security: Open Auth, phy: bgn, rssi: -17
I (69674) wifi:pm start, type: 1

I (69674) wifi_cmd: Connected to csi_softap (bssid: 30:ae:a4:00:4b:91, channel: 13)
I (69724) wifi:AP's beacon interval = 102400 us, DTIM period = 2
I (70634) esp_netif_handlers: sta ip: 192.168.4.2, mask: 255.255.255.0, gw: 192.168.4.1
```

3. Configure device B to send a Ping packet to the ip gateway of device A
```shell
csi> ping 192.168.4.1
```

4. Configure the filter condition of the CSI data packet of device B, the byte of the CSI data packet is: 384, and the filtered device is: device A

```shell
csi> csi -l 384 -m 30:ae:a4:00:4b:91
I (526744) app_main: <1> time: 1030 ms, rssi: -26, corr: 0.959, std: 0.008, std_avg: 0.000, std_max: 0.000, threshold: 0.300/1.500, trigger: 0/0, free_heap: 134928/137440
I (527694) app_main: <2> time: 940 ms, rssi: -27, corr: 0.819, std: 0.045, std_avg: 0.000, std_max: 0.000, threshold: 0.300/1.500, trigger: 0/0, free_heap: 134712/137240
I (528644) app_main: <3> time: 930 ms, rssi: -26, corr: 0.974, std: 0.008, std_avg: 0.000, std_max: 0.000, threshold: 0.300/1.500, trigger: 0/0, free_heap: 134328/137148
I (529544) app_main: <4> time: 900 ms, rssi: -26, corr: 0.940, std: 0.013, std_avg: 0.000, std_max: 0.000, threshold: 0.300/1.500, trigger: 0/0, free_heap: 134164/137240
```

5. Configure device B's human body movement threshold. When `std` is greater than the threshold, human body movement will be triggered
```shell
I (1263704) app_main: <707> time: 1140 ms, rssi: -26, corr: 0.969, std: 0.006, std_avg: 0.009, std_max: 0.016, threshold: 0.018/1.500, trigger: 0/0, free_heap: 100672/137084
I (1264664) app_main: <708> time: 950 ms, rssi: -26, corr: 0.979, std: 0.006, std_avg: 0.009, std_max: 0.016, threshold: 0.018/1.500, trigger: 0/0, free_heap: 100672/137084
csi> detect -a 0.017
I (1265704) app_main: <709> time: 1030 ms, rssi: -26, corr: 0.968, std: 0.008, std_avg: 0.009, std_max: 0.016, threshold: 0.017/1.500, trigger: 0/0, free_heap: 100672/137168
I (1266784) app_main: <710> time: 1070 ms, rssi: -26, corr: 0.965, std: 0.007, std_avg: 0.009, std_max: 0.016, threshold: 0.017/1.500, trigger: 0/0, free_heap: 100672/136740
I (1267854) app_main: <711> time: 1050 ms, rssi: -26, corr: 0.961, std: 0.008, std_avg: 0.009, std_max: 0.016, threshold: 0.017/1.500, trigger: 0/0, free_heap: 100672/137168
I (1268884) app_main: <712> time: 1020 ms, rssi: -27, corr: 0.969, std: 0.012, std_avg: 0.009, std_max: 0.016, threshold: 0.017/1.500, trigger: 0/0, free_heap: 100672/136992
W (1270104) app_main: Someone is moving
I (1270104) app_main: <713> time: 1170 ms, rssi: -27, corr: 0.920, std: 0.047, std_avg: 0.009, std_max: 0.018, threshold: 0.017/1.500, trigger: 1/0, free_heap: 100672/137168
```

6. You can also print the original CSI data
```
csi> csi -o
type,id,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,len,first_word,data
CSI_DATA,0,30:ae:a4:00:4b:91,-27,11,1,7,1,1,1,1,0,0,0,-89,1,13,2,1826877250,0,67,0,384,1,[ 67 48 4 0 0 0 0 0 0 0 3 -1 13 -2 13 -3 13 -3 13 -3 13 -4 13 -4 13 -4 13 -4 13 -5 13 -5 13 -5 13 -5 13 -5 12 -6 12 -6 12 -6 12 -7 12 -7 12 -7 12 -8 12 -8 12 -8 12 -8 12 -9 12 -9 12 -9 6 -5 12 -11 12 -11 12 -11 12 -11 12 -12 12 -12 12 -12 12 -13 12 -13 13 -13 13 -13 14 -13 14 -13 14 -13 14 -13 15 -14 15 -14 15 -14 16 -14 16 -13 16 -13 17 -13 17 -12 18 -12 19 -11 19 -11 5 -3 0 0 0 0 0 0 0 0 1 0 4 0 25 3 25 3 25 3 25 2 25 -1 25 -2 25 -2 25 -3 25 -3 25 -4 25 -4 25 -5 25 -5 25 -6 25 -6 25 -7 24 -7 25 -8 24 -8 24 -9 24 -9 24 -10 24 -11 24 -11 24 -11 24 -12 24 -13 24 -13 24 -14 24 -15 26 -12 24 -16 24 -17 25 -17 25 -18 25 -18 25 -19 25 -20 26 -20 26 -20 27 -20 28 -21 28 -21 29 -21 29 -22 30 -22 30 -22 31 -21 32 -21 32 -21 33 -20 34 -20 35 -20 35 -19 36 -18 37 -17 38 -16 11 -5 2 -1 -1 0 -2 0 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -4 5 -15 22 -16 22 -16 22 -17 21 -18 21 -18 20 -18 20 -18 19 -19 19 -19 19 -19 18 -19 18 -19 18 -19 18 -19 17 -19 17 -19 17 -19 16 -18 16 -18 16 -18 16 -17 16 -17 16 -17 16 -16 16 -16 17 -17 15 -15 17 -14 17 -14 17 -14 17 -13 17 -13 17 -12 18 -12 18 -12 18 -11 19 -11 19 -10 19 -10 20 -9 20 -9 20 -8 20 -8 21 -8 21 -7 21 -7 21 -6 22 -6 22 -6 22 -5 23 -5 23 -5 23 -7 23 -7 23 -6 24 -6 24 -1 3 ]
CSI_DATA,1,30:ae:a4:00:4b:91,-27,11,1,7,1,1,1,1,0,0,0,-89,1,13,2,1827007808,0,67,0,384,1,[ 67 48 4 0 0 0 0 0 0 0 1 2 6 11 6 11 6 11 6 11 6 11 7 11 7 11 7 11 7 10 8 10 8 11 8 10 8 10 8 10 9 10 9 9 9 9 9 9 10 9 10 9 10 9 10 9 11 9 11 9 11 8 12 8 6 4 12 8 13 8 13 8 13 8 13 8 14 8 14 8 14 8 15 8 15 8 15 9 15 9 16 9 16 9 16 10 16 10 16 11 17 11 16 11 16 12 16 12 16 13 16 13 15 14 15 15 15 15 3 3 0 0 0 0 0 0 0 0 -1 -1 0 2 8 23 9 23 9 23 10 23 10 22 10 22 11 22 11 22 12 22 12 22 13 21 13 21 13 21 14 21 14 20 15 20 15 20 16 20 16 19 16 19 17 19 17 19 18 18 18 18 19 18 19 18 20 18 21 17 21 17 22 17 22 17 23 17 23 17 24 17 24 16 25 16 26 17 26 17 27 17 28 17 28 17 29 18 29 19 29 19 30 19 30 20 30 21 30 21 30 22 30 23 30 23 30 24 30 26 29 27 29 28 28 29 28 29 6 7 -1 -1 -1 -1 -1 -1 0 0 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -7 -1 -28 -3 -28 -4 -28 -5 -28 -5 -27 -6 -27 -7 -26 -7 -27 -8 -26 -9 -26 -9 -26 -9 -26 -9 -25 -9 -25 -9 -25 -9 -24 -9 -24 -9 -23 -9 -23 -9 -23 -9 -23 -9 -23 -9 -23 -8 -23 -8 -23 -7 -23 -7 -22 -7 -22 -6 -22 -6 -22 -6 -22 -5 -22 -5 -22 -4 -22 -3 -23 -3 -23 -2 -23 -2 -23 -2 -23 -1 -23 0 -23 0 -23 1 -23 1 -23 1 -23 2 -23 2 -23 3 -23 4 -24 4 -24 4 -24 5 -24 5 -24 5 -24 5 -24 5 -24 6 -24 6 -4 0 ]
CSI_DATA,2,30:ae:a4:00:4b:91,-27,11,1,7,1,1,1,1,0,0,0,-89,1,13,2,1827121360,0,67,0,384,1,[ 67 48 4 0 0 0 0 0 0 0 1 -3 7 -12 7 -12 7 -12 6 -12 6 -12 6 -12 6 -12 5 -13 5 -13 5 -13 5 -13 5 -13 5 -13 4 -13 4 -14 4 -14 4 -14 3 -14 3 -14 3 -14 3 -14 3 -15 2 -14 2 -15 2 -15 2 -15 1 -8 1 -16 1 -16 1 -17 1 -17 1 -17 1 -17 1 -17 1 -18 1 -18 1 -18 1 -19 1 -19 1 -19 1 -20 2 -20 2 -20 2 -20 3 -20 3 -21 3 -21 4 -21 5 -21 5 -21 6 -21 6 -21 7 -21 1 -6 0 0 0 0 0 0 0 0 0 -1 3 -4 18 -19 17 -19 17 -19 17 -19 15 -21 14 -21 14 -21 14 -22 14 -22 13 -22 13 -22 12 -22 12 -23 12 -23 11 -23 11 -24 10 -24 10 -24 10 -25 10 -25 9 -25 9 -25 8 -25 8 -26 8 -26 7 -26 7 -27 6 -27 6 -28 6 -28 8 -29 5 -29 5 -30 5 -30 5 -31 5 -31 4 -32 4 -32 4 -33 4 -33 4 -34 5 -35 5 -36 5 -36 6 -36 6 -37 7 -37 7 -38 8 -38 9 -38 10 -38 11 -38 12 -38 13 -38 14 -38 15 -38 16 -38 5 -12 1 -3 -1 0 -1 1 -1 -1 0 0 0 0 -1 -1 -1 -1 -1 -1 3 5 12 23 11 23 11 24 10 24 9 24 9 25 8 25 8 25 7 25 6 24 6 24 6 24 6 24 5 24 5 24 5 24 4 24 4 23 4 23 4 23 4 22 5 22 5 22 5 22 5 22 5 21 4 21 6 21 6 21 6 20 7 20 7 19 8 19 8 19 8 19 8 19 9 18 9 18 10 19 11 18 11 18 11 18 12 18 12 18 13 17 13 17 13 17 14 17 14 17 15 17 15 17 15 17 16 17 15 18 15 18 16 18 16 18 1 2 ]
CSI_DATA,3,30:ae:a4:00:4b:91,-27,11,1,7,1,1,1,1,0,0,0,-89,1,13,2,1827137218,0,67,0,384,1,[ 67 48 4 0 0 0 0 0 0 0 2 3 8 12 8 12 8 12 9 12 9 12 9 11 9 11 10 11 10 11 10 11 11 11 11 10 11 10 11 10 11 10 11 10 12 9 12 9 12 9 12 9 13 9 13 8 13 8 13 8 14 8 14 8 7 4 15 7 15 7 16 7 16 7 16 7 17 7 17 7 17 7 18 7 18 7 18 8 19 8 19 8 19 8 19 9 20 9 20 9 20 10 20 10 20 11 20 11 20 12 20 13 20 13 19 14 19 15 4 3 0 0 0 0 0 0 0 0 0 1 2 4 15 23 16 23 16 23 17 23 16 22 17 22 17 22 17 22 17 22 18 22 18 21 19 21 19 21 20 20 20 20 21 19 21 19 22 19 22 18 23 18 23 17 23 17 24 17 24 16 25 16 25 16 26 15 26 15 27 15 28 14 29 13 29 13 29 13 30 13 31 13 31 13 32 13 33 13 34 13 34 13 35 14 36 14 36 14 37 14 37 15 38 16 38 16 38 17 38 18 39 19 39 20 39 21 39 22 38 24 38 25 37 26 37 26 11 8 1 1 -1 -1 -2 -1 0 0 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -8 0 -31 1 -31 1 -31 0 -31 0 -31 -2 -31 -3 -31 -3 -31 -4 -31 -5 -30 -5 -29 -4 -29 -5 -29 -5 -29 -5 -29 -5 -28 -6 -28 -5 -29 -5 -29 -5 -29 -6 -27 -6 -25 -6 -27 -6 -28 -3 -27 -2 -27 -2 -28 -1 -27 -2 -26 -2 -25 -2 -25 -1 -25 0 -26 0 -26 0 -26 0 -27 1 -26 2 -25 3 -25 4 -26 5 -25 5 -25 5 -25 6 -25 6 -25 7 -25 7 -25 8 -26 8 -25 9 -26 9 -25 10 -25 10 -25 11 -24 12 -24 13 -25 13 -26 13 -4 1 ]
```