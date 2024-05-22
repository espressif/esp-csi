# Adding Wi-Fi CSI Functionality in ESP RainMaker [[中文]](./README_cn.md)

## Build and Flashing Instructions
Follow the ESP RainMaker documentation [Getting Started](https://rainmaker.espressif.com/docs/get-started.html) section to build and flash the firmware.

## How to Use This Example

### Parameter Description
- **someone_status**: false - no one, true - someone
- **someone_timeout**: If someone moves within this time in the room, it will be marked as someone present. Time is in seconds.
- **move_status**: false - no movement, true - movement
- **move_count**: Number of times movement is detected between the last ESP RainMaker report.
- **move_threshold**: Threshold value to determine if there is movement.
- **filter_window**: Size of the buffer queue for Wi-Fi CSI waveform jitter values, used for filtering outliers.
- **filter_count**: If the jitter value of the Wi-Fi CSI waveform exceeds the `move_threshold` for `filter_count` times within the buffer queue, it is marked as movement detected.
- **threshold_calibrate**: Enable threshold auto-calibration.
- **threshold_calibrate_timeout**: Auto-calibration timeout, in seconds.

### App Version
- ESP RainMaker App: [v2.11.1](https://m.apkpure.com/p/com.espressif.rainmaker)+
> Note: `ESP RainMaker` App version before 2.11.1 does not support `time series` function, `move_count` cannot be displayed normally

### App Operation
1. Open the RainMaker App.
2. Click on `+` to add a device.
3. Wait for the device to connect to the cloud.
4. Enable `threshold_calibrate` to perform auto-calibration. Ensure there is no one or no movement in the room during calibration.
5. After calibration, the threshold value for movement detection will be displayed in `move_threshold`, and `move_status` will become true when movement is detected.

### Device
- [x] ESP32-S3-DevKitC-1
- [x] ESP32-C3-DevKitC

### Device Operation
1. **Factory Reset**: Press and hold the `BOOT` button for more than 5 seconds to reset the development board to factory defaults.

## Device Status
- Human Movement Detection
    - Green LED indicates movement detected in the room.
    - White LED indicates no movement detected in the room.
 
- Human Presence Detection
    > The current algorithm for human presence detection is not ideal. Therefore, the presence of someone is determined by whether there has been any movement within 1 minute. If there is movement, it is considered someone is present; otherwise, it is considered no one is present.
    - LED lights up when there is someone in the room.
    - LED turns off when there is no one in the room.

- Human Movement Detection Threshold
    > - The threshold for human movement detection can be set via the mobile app or obtained through auto-calibration. If not set, the default value will be used.
    > - During calibration, ensure there is no one or no movement in the room. After calibration, the detection sensitivity will be increased. However, if there is movement in the room, it may result in false detection. Therefore, it is recommended to perform calibration when there is no one in the room.
    > - The calibrated threshold will be saved in NVS and will be used after the next reboot.
    - During human movement threshold calibration, the LED will flash yellow.

## Common Issues

### RainMaker Reporting Failure
------
- **Issue**: The device-side logs show the following error:
    ```shell
    E (399431) esp_rmaker_mqtt: Out of MQTT Budget. Dropping publish message.
    ```

- **Cause**: The amount of data being reported by the device exceeds the limit of `ESP RainMaker`.

------
- **Issue**: Continuous movement detection without actual movement or the device does not detect any movement.

- **Solution**:
  1. Incorrect human movement detection threshold leading to false recognition.
     - The default Wi-Fi CSI human movement detection threshold may not meet the actual requirements. Adjust it according to the actual situation or enable auto-calibration through the mobile app.
     - The default outlier filtering window size for Wi-Fi CSI may not meet the actual requirements. Adjust it according to the actual situation through the mobile app.

  2. Unstable network causing inaccurate detection.
     - Check if it works properly after replacing the router.
     - Try placing the router in a more suitable location.

  3. The above methods still cannot solve the problem, modify the LOG level and submit [issue](https://github.com/espressif/esp-csi/issues) on github
     ```c
     esp_log_level_set("esp_radar", ESP_LOG_DEBUG);
     ```