# Get Started Examples[[中文]](./README_cn.md)

This example demonstrates how to obtain CSI data through communication between two espressif chips, and uses a graphical interface to display real-time data of CSI subcarriers

## hardware

You need to prepare two development boards for espressif chips, one as the sender and the other as the receiver

![example_display](./docs/_static/example_display.png)

1. Use ESP32-C5 / ESP32-S6: ESP32-C5 supports dual-band Wi-Fi communication and is one of the best RF chips available. ESP32-C6 is the best RF chip among the currently released models.
2. Use an external antenna: The PCB antenna has poor directivity and is easily interfered with by the motherboard.
3. The distance between the two devices should be greater than 1 meter.

## Binding

1. Burn the firmware of `csi_recv` and `csi_send` on two development boards respectively

    ![device_log](./docs/_static/device_log.png)

    ```shell
    # csi_send
    cd esp-csi/examples/get-started/csi_send
    idf.py set-target esp32c3
    idf.py flash -b 921600 -p /dev/ttyUSB0 monitor

    # csi_recv
    cd esp-csi/examples/get-started/csi_recv
    idf.py set-target esp32c3
    idf.py flash -b 921600 -p /dev/ttyUSB1
    ```

2. Run `csi_data_read_parse.py` in `csi_recv` for data analysis. Please close `idf.py monitor` before running

    ```shell
    cd esp-csi-gitlab/examples/get-started/tools

    # Install python related dependencies
    pip install -r requirements.txt

    # Graphical display
    python csi_data_read_parse.py -p /dev/ttyUSB1
    ```

## CSI Data Format

Taking a line of CSI raw data as an example:

> type,id,mac,rssi,rate,sig_mode,mcs,bandwidth,smoothing,not_sounding,aggregation,stbc,fec_coding,sgi,noise_floor,ampdu_cnt,channel,secondary_channel,local_timestamp,ant,sig_len,rx_state,len,first_word,data
CSI_DATA,0,94:d9:b3:80:8c:81,-30,11,1,6,1,0,1,0,1,0,0,-93,0,13,2,2751923,0,67,0,128,1,"[67,48,4,0,0,0,0,0,0,0,5,0,20,1,20,1,19,0,17,1,16,2,15,2,14,1,12,0,12,-1,12,-3,12,-4,13,-6,15,-7,16,-8,16,-8,16,-8,16,-6,15,-5,15,-4,14,-4,13,-4,12,-4,11,-4,10,-4,9,-5,8,-6,4,-4,8,-9,9,-10,9,-10,10,-11,11,-10,11,-10,12,-9,11,-8,11,-7,10,-6,9,-6,7,-6,6,-7,5,-7,5,-8,5,-9,5,-10,5,-11,5,-11,6,-11,7,-11,8,-11,9,-10,9,-9,8,-8,8,-7,1,-2,0,0,0,0,0,0,0,0]"

*ESP32-C5 and ESP32-C6 are:*
>type,seq,mac,rssi,rate,noise_floor,fft_gain,agc_gain,channel,local_timestamp,sig_len,rx_state,len,first_word,data
CSI_DATA,7,1a:00:00:00:00:00,-23,11,-96,32,4,11,372852,47,0,256,0,"[0,0,0,0,0,0,0,0,0,0,0,0,-6,-13,-6,-14,-3,-15,-2,-16,-2,-18,-3,-17,-1,-18,2,-19,0,-21,3,-21,1,-20,4,-21,4,-23,6,-22,7,-21,8,-23,9,-23,10,-21,10,-22,11,-20,12,-19,11,-25,13,-22,12,-23,14,-23,14,-22,14,-21,13,-21,13,-19,14,-22,14,-19,14,-23,16,-22,14,-22,13,-22,14,-21,13,-22,13,-23,13,-23,12,-26,13,-24,13,-24,12,-25,14,-29,13,-26,14,-26,15,-26,13,-27,15,-28,16,-27,14,-30,15,-28,16,-28,18,-27,16,-31,18,-31,19,-31,0,0,0,0,0,0,-29,-23,-28,-21,-30,-18,-26,-20,-30,-21,-25,-23,-26,-21,-25,-22,-26,-19,-22,-22,-24,-19,-22,-20,-24,-20,-24,-18,-23,-18,-22,-18,-25,-17,-23,-18,-21,-18,-21,-17,-24,-14,-22,-16,-21,-14,-22,-15,-21,-19,-23,-16,-22,-17,-23,-13,-23,-16,-25,-15,-21,-17,-22,-15,-21,-17,-23,-16,-20,-16,-21,-20,-21,-19,-21,-19,-19,-20,-17,-20,-18,-20,-16,-21,-15,-21,-15,-20,-13,-21,-11,-20,-10,-20,-11,-19,-9,-20,-8,-22,-6,-19,-7,-20,-4,-19,-2,-18,-2,-18,1,-17,4,-18,0,0,0,0,0,0,0,0,0,0]"

- **Metadata Fields**: Including type, id, mac, rssi, rate, ... len, first_word, etc.
- **CSI Data**: Stored in the last item data array, enclosed in [...]. It contains the channel state information for each subcarrier. For detailed structure, refer to the Long Training Field (LTF) section of the [ESP-WIFI-CSI Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-channel-state-information). For each subcarrier, the imaginary part is stored first, followed by the real part (i.e., [Imaginary part of subcarrier 1, Real part of subcarrier 1, Imaginary part of subcarrier 2, Real part of subcarrier 2, Imaginary part of subcarrier 3, Real part of subcarrier 3, ...]).
The order of LTF is: LLTF, HT-LTF, STBC-HT-LTF. Depending on the channel and grouping information, not all 3 LTFs may appear.

## A&Q

### 1. `csi_send` prints no memory
- **Phenomenon**: The following log appears on the serial port:
  ```shell
    W (510693) csi_send: <ESP_ERR_ESPNOW_NO_MEM> ESP-NOW send error
  ````
- **Reason**: The current channel is congested causing congestion in sending packets, so that the ESP-NOW buffer space is full
- **Solution**: Change the Wi-Fi channel or change to a place with a better network environment

### 2. `csi_data_read_parse.py` Serial port printing exception
- **Phenomenon**: The following log appears on the serial port:
    ```shell
        element number is not equal
        data is not incomplete
    ````
- **Reason**: PYQT takes up a lot of CPU when drawing, causing the PC to be unable to read the serial port buffer queue in time, resulting in data confusion
- **Solution**: Advance the baud rate of the serial port
