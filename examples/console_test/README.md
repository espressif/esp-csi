# CSI Test Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

## Introduction

You can control/interact with the interactive shell running on the ESP32 through the serial port (UART) to test the CSI. Here are the functions of this example.

1. Provide CSI information.
1. Output CSI raw data
1. Configuration for human movement detection

> * CSI data can be obtained from any wifi packet which carried csi information. **For best experience, we strongly recommand you use ESP32 serial device rather than router in csi demo.**
> * For the usage of creating an interactive shell on the ESP32, see: [Console Component](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/console.html#console)

## How to use examples

This example can be build in esp-idf version higher than 4.3.

### Hardware Required

This example can run on any ESP32 development board. At least two development boards should be prepared, and the two development boards should be placed perpendicular to the ground during testing.
If you only have one device, you can use the router to act as device A.

### Demo steps to test

1. Compile the project (This project only support CMAKE).
1. Download the firmware to two ESP32 devices separately.
1. Configure the device A in SoftAp mode.

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

1. Set device B to Sta mode and make it connect to device A

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

1. Set the filter condition of the CSI data packet of device B, the size of the CSI data packet is: 384, and the filtered device is: MAC address of device A(AP mode).

    ```shell
    csi> csi -l 384 -m 30:ae:a4:00:4b:91
    ```

1. let device B to send a Ping packet to device A.

    ```shell
    csi> ping 192.168.4.1
    I (526744) app_main: <1> time: 1030 ms, rssi: -26, corr: 0.959, std: 0.008, std_avg: 0.000, std_max: 0.000, threshold: 0.300/1.500, trigger: 0/0, free_heap: 134928/137440
    I (527694) app_main: <2> time: 940 ms, rssi: -27, corr: 0.819, std: 0.045, std_avg: 0.000, std_max: 0.000, threshold: 0.300/1.500, trigger: 0/0, free_heap: 134712/137240
    I (528644) app_main: <3> time: 930 ms, rssi: -26, corr: 0.974, std: 0.008, std_avg: 0.000, std_max: 0.000, threshold: 0.300/1.500, trigger: 0/0, free_heap: 134328/137148
    I (529544) app_main: <4> time: 900 ms, rssi: -26, corr: 0.940, std: 0.013, std_avg: 0.000, std_max: 0.000, threshold: 0.300/1.500, trigger: 0/0, free_heap: 134164/137240
    ```

1. Configure device B's human body movement threshold. When `std` is greater than the threshold, human body movement will be triggered

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

1. You can also print the original CSI raw data if you want.
    > The default baudrate can not carry such a large amount of data, See **How to change esp32 console baudrate**
    > If you want to save device log to file, press "CTRL+t", "CTRL+l". See [IDF Monitor](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-monitor.html)
    > The csi raw data can be analyzed by some python scripts in tools folder. See [tools/README.md](tools/README.md)

    ```shell
    csi> csi -o
    type,id,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,len,first_word,data
    CSI_DATA,0,ac:67:b2:53:8f:31,-31,11,1,7,1,1,1,0,0,0,0,-89,0,13,2,48079152,0,66,0,384,1,"[66,32,4,0,0,0,0,0,0,0,-3,-3,-12,-9,-12,-9,-12,-9,-12,-8,-12,-8,-12,-8,-12,-8,-12,-7,-12,-7,-12,-7,-13,-6,-13,-6,-13,-6,-13,-5,-13,-5,-13,-5,-13,-4,-13,-4,-14,-4,-14,-4,-14,-4,-14,-3,-14,-3,-15,-3,-15,-2,-15,-2,-8,-1,-16,-2,-16,-2,-16,-2,-16,-1,-17,-1,-17,-2,-17,-2,-17,-2,-18,-2,-18,-1,-18,-1,-19,-1,-19,-2,-20,-2,-20,-2,-20,-3,-20,-3,-20,-3,-21,-4,-21,-5,-21,-6,-20,-6,-20,-6,-21,-7,-21,-7,-21,-8,-6,-2,0,0,0,0,0,0,0,0,-1,-1,-3,-3,-20,-21,-19,-20,-20,-20,-20,-20,-22,-18,-22,-18,-21,-18,-21,-17,-21,-17,-22,-16,-22,-16,-22,-16,-22,-15,-22,-14,-23,-13,-23,-12,-24,-12,-24,-12,-24,-11,-24,-11,-24,-10,-25,-10,-25,-9,-25,-9,-25,-9,-26,-8,-27,-7,-27,-6,-28,-6,-28,-5,-29,-7,-29,-5,-30,-6,-30,-5,-30,-5,-31,-5,-31,-5,-32,-5,-32,-5,-33,-5,-33,-5,-34,-5,-35,-5,-36,-5,-36,-6,-37,-7,-37,-7,-37,-8,-38,-9,-38,-10,-38,-11,-38,-12,-38,-13,-37,-14,-38,-15,-38,-16,-38,-17,-10,-5,-1,-1,-1,-1,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,-5,21,-19,21,-18,22,-18,23,-17,23,-16,24,-16,24,-15,25,-14,25,-14,25,-13,25,-13,25,-13,25,-13,25,-12,25,-12,25,-11,25,-11,25,-11,25,-11,25,-11,25,-11,25,-11,24,-11,24,-11,24,-11,24,-11,24,-10,24,-11,23,-11,23,-11,22,-12,22,-12,22,-12,22,-12,22,-12,22,-13,21,-13,21,-14,21,-14,21,-15,20,-15,20,-15,20,-15,20,-16,20,-16,19,-16,19,-17,19,-17,19,-17,18,-18,18,-19,18,-19,18,-20,20,-19,20,-19,20,-19,19,-19,2,-3]"
    CSI_DATA,1,ac:67:b2:53:8f:31,-29,11,1,6,1,1,1,1,0,0,0,-89,1,13,2,48098948,0,67,0,384,1,"[67,48,4,0,0,0,0,0,0,0,0,-4,0,-15,0,-15,0,-15,0,-15,-1,-15,-1,-15,-1,-15,-1,-15,-2,-15,-2,-15,-2,-15,-3,-14,-3,-14,-4,-14,-4,-14,-4,-14,-4,-14,-5,-14,-5,-14,-6,-14,-6,-14,-6,-14,-7,-14,-7,-14,-8,-14,-8,-14,-4,-7,-9,-14,-9,-14,-9,-14,-10,-14,-10,-15,-10,-15,-10,-15,-11,-15,-11,-16,-11,-16,-11,-16,-11,-16,-11,-17,-11,-17,-12,-17,-12,-18,-11,-18,-11,-19,-11,-19,-11,-20,-10,-20,-10,-20,-9,-21,-9,-21,-8,-21,-8,-21,-2,-6,0,0,0,0,0,0,0,0,-1,-1,0,-4,8,-29,8,-28,8,-28,7,-29,3,-29,3,-29,3,-28,3,-28,2,-28,1,-28,0,-28,0,-28,-1,-27,-1,-27,-2,-27,-3,-27,-3,-27,-3,-27,-4,-27,-5,-27,-6,-27,-7,-27,-7,-26,-8,-27,-9,-27,-10,-27,-10,-27,-11,-27,-11,-27,-12,-27,-11,-29,-14,-27,-14,-28,-15,-28,-15,-28,-16,-28,-16,-29,-17,-29,-18,-30,-18,-30,-18,-31,-18,-32,-18,-33,-18,-33,-18,-34,-18,-35,-18,-35,-18,-35,-18,-36,-17,-37,-17,-38,-16,-38,-15,-39,-14,-40,-12,-40,-11,-41,-10,-41,-3,-11,-1,-1,0,0,0,0,-1,-1,0,0,0,0,-1,-1,-1,-1,-1,-1,6,3,25,13,25,14,24,15,24,16,23,17,23,17,23,17,23,18,22,19,22,19,21,19,21,19,21,20,21,20,20,20,20,20,20,19,20,19,20,19,20,19,20,19,20,19,20,19,20,18,20,18,20,18,18,20,20,17,20,17,20,17,20,16,20,16,21,15,21,15,21,14,21,14,21,14,21,13,22,13,22,13,23,13,23,12,23,11,23,11,23,10,24,9,24,9,24,8,25,8,25,8,25,8,26,8,26,7,26,10,26,10,26,10,26,9,3,1]"
    CSI_DATA,2,ac:67:b2:53:8f:31,-29,11,1,6,1,1,1,1,0,0,0,-89,1,13,2,48111203,0,67,0,384,1,"[67,48,4,0,0,0,0,0,0,0,-2,-3,-7,-12,-7,-12,-7,-12,-7,-12,-7,-12,-7,-12,-7,-12,-8,-11,-8,-11,-8,-11,-8,-10,-9,-10,-9,-10,-9,-10,-10,-10,-10,-10,-10,-9,-10,-9,-10,-9,-11,-9,-11,-9,-11,-9,-11,-8,-12,-8,-12,-8,-12,-8,-7,-4,-13,-8,-13,-8,-14,-8,-14,-8,-14,-8,-15,-8,-15,-8,-15,-8,-15,-8,-15,-8,-16,-8,-16,-9,-16,-9,-16,-10,-17,-10,-17,-10,-17,-10,-17,-11,-17,-11,-16,-12,-16,-12,-16,-13,-16,-13,-16,-14,-15,-14,-15,-15,-4,-4,0,0,0,0,0,0,0,0,-1,-1,-2,-4,-11,-25,-10,-25,-11,-25,-11,-24,-12,-24,-12,-24,-13,-23,-13,-23,-13,-22,-13,-22,-13,-22,-14,-22,-14,-21,-15,-20,-15,-20,-16,-19,-16,-19,-17,-19,-17,-19,-18,-18,-18,-18,-18,-17,-18,-17,-19,-17,-19,-17,-20,-17,-21,-16,-22,-16,-23,-15,-23,-15,-24,-15,-24,-15,-24,-15,-25,-15,-25,-15,-26,-15,-26,-16,-27,-16,-28,-16,-28,-16,-29,-16,-29,-17,-30,-17,-30,-18,-30,-18,-30,-19,-31,-20,-31,-20,-31,-21,-30,-22,-30,-23,-30,-24,-29,-25,-29,-26,-29,-26,-28,-27,-28,-28,-7,-8,-1,-1,-1,-1,-1,-1,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,6,-2,25,-7,25,-6,26,-5,26,-4,26,-4,27,-3,27,-2,27,-1,27,-1,26,-1,26,-1,26,-1,26,0,26,0,26,1,26,1,25,1,25,1,25,2,25,2,25,2,25,1,25,1,25,1,25,1,25,0,25,0,24,0,24,0,24,0,24,-1,24,-1,24,-1,24,-1,24,-1,23,-2,23,-3,23,-3,23,-4,23,-4,23,-4,23,-5,23,-5,23,-6,23,-6,23,-6,23,-7,23,-7,23,-7,23,-8,23,-9,23,-10,23,-10,24,-10,24,-10,24,-10,24,-10,2,-2]"
    ```

## How to change esp32 console baudrate

1. `Component config` -> `Common ESP-related` -> `UART console baud rate`, Set it to 2000000.
1. `Serial flasher config` -> `idf.py monitor' baud rate`, Select "2 Mbps"
1. Open "components/console/esp_console_repl.c", find the definition of esp_console_new_repl_uart(), change `.source_clk = UART_SCLK_REF_TICK` to `.source_clk = UART_SCLK_APB`
