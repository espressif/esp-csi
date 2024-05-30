# ESP-CSI [[中文]](./README_cn.md)

## Introduction to CSI

Channel State Information (CSI) is an important parameter that describes the characteristics of a wireless channel, including indicators such as signal amplitude, phase, and signal delay. In Wi-Fi communication, CSI is used to measure the state of the wireless network channel. By analyzing and studying changes in CSI, one can infer physical environmental changes that cause channel state changes, achieving non-contact intelligent sensing. CSI is very sensitive to environmental changes. It can sense not only large movements such as people or animals walking and running but also subtle actions in a static environment, such as breathing and chewing. These capabilities make CSI widely applicable in smart environment monitoring, human activity monitoring, wireless positioning, and other applications.

## Basic Knowledge

To better understand CSI technology, we provide some related basic knowledge documents (to be updated gradually):

- [Signal Processing Fundamentals](./docs/en/Signal-Processing-Fundamentals.md)
- [OFDM Introduction](./docs/en/OFDM-introduction.md)
- [Wireless Channel Fundamentals](./docs/en/Wireless-Channel-Fundamentals.md)
- [Introduction to Wireless Location](./docs/en/Introduction-to-Wireless-Location.md)
- [Wireless Indicators CSI and RSSI](./docs/en/Wireless-indicators-CSI-and-RSSI.md)
- [CSI Applications](./docs/en/CSI-Applications.md)

## Advantages of Espressif CSI

- **Full series support:** All ESP32 series support CSI, including ESP32 / ESP32-S2 / ESP32-C3 / ESP32-S3 / ESP32-C6.
- **Strong ecosystem:** Espressif is a global leader in the Wi-Fi MCU field, perfectly integrating CSI with existing IoT devices.
- **More information:** ESP32 provides rich channel information, including RSSI, RF noise floor, reception time, and the 'rx_ctrl' field of the antenna.
- **Bluetooth assistance:** ESP32 also supports BLE, for example, it can scan surrounding devices to assist detection.
- **Powerful processing capability:** The ESP32 CPU is dual-core 240MHz, supporting AI instruction sets, capable of running machine learning and neural networks.
- **OTA upgrade:** Existing projects can upgrade to new CSI features through software OTA without additional hardware costs.

## Example Introduction

### [get-started](./examples/get-started)

Helps users quickly get started with CSI functionality, demonstrating the acquisition and initial analysis of CSI data through basic examples. For details, see [README](./examples/get-started/README.md).

- [csi_recv](./examples/get-started/csi_recv) demonstrates the ESP32 as a receiver example.
- [csi_send](./examples/get-started/csi_send) demonstrates the ESP32 as a sender example.
- [csi_recv_router](./examples/get-started/csi_recv_router) demonstrates using a router as the sender, with the ESP32 triggering the router to send CSI packets via Ping.
- [tools](./examples/get-started/tools) provides scripts for assisting CSI data analysis, such as csi_data_read_parse.py.

### [esp-radar](./examples/esp-radar)

Provides some applications using CSI data, including RainMaker cloud reporting and human activity detection.

- [connect_rainmaker](./examples/esp-radar/connect_rainmaker) demonstrates capturing CSI data and uploading it to Espressif's RainMaker cloud platform.
- [console_test](./examples/esp-radar/console_test) demonstrates an interactive console that allows dynamic configuration and capture of CSI data, with applications for human activity detection algorithms.

## How to get CSI

### 4.1 Get router CSI

<img src="docs/_static/get_router_csi.png" width="550">

- **How ​​to implement:** ESP32 sends a Ping packet to the router, and receives the CSI information carried in the Ping Replay returned by the router.
- **Advantage:** Only one ESP32 plus router can be completed.
- **Disadvantages:** Depends on the router, such as the location of the router, the supported Wi-Fi protocol, etc.
- **Applicable scenario:** There is only one ESP32 in the environment, and there is a router in the detection environment.

### 4.2 Get CSI between devices

<img src="docs/_static/get_device_csi.png" width="550">

- **How ​​to implement:** ESP32 A and B both send Ping packets to the router, and ESP32 A receives the CSI information carried in the Ping Replay returned by ESP32 B, which is a supplement to the first detection scenario.
- **Advantage:** Does not depend on the location of the router, and is not affected by other devices connected under the router.
- **Disadvantage:** Depends on the Wi-Fi protocol supported by the router, environment.
- **Applicable scenario:** There must be more than two ESP32s in the environment.

### 4.3 Get CSI specific devices

<img src="docs/_static/get_broadcast_csi.png" width="550">

- **How ​​to implement:** The packet sending device continuously switches channels to send out packets. ESP32 A, B, and C all obtain the CSI information carried in the broadcast packet of the packet sending device. This method has the highest detection accuracy and reliability.
- **Advantages:** The completion is not affected by the router, and the detection accuracy is high. When there are multiple devices in the environment, only one packet sending device will cause little interference to the network environment.
- **Disadvantages:** In addition to the ordinary ESP32, it is also necessary to add a special package issuing equipment, the cost is the same and higher.
- **Applicable scenarios:** Suitable for scenarios that require high accuracy and multi-device cluster positioning.

## 5 Note

1. The effect of external IPEX antenna is better than PCB antenna, PCB antenna has directivity.
2. Test in an unmanned environment. Avoid the influence of other people's activities on test results.

## 6 Related resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html) is the documentation for the Espressif IoT development framework.
- [ESP-WIFI-CSI Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-channel-state-information) is the use of ESP-WIFI-CSI Description.
- If you find a bug or have a feature request, you can submit it on [Issues](https://github.com/espressif/esp-csi/issues) on GitHub. Please check to see if your question already exists in the existing Issues before submitting it.

## Reference

1. [Through-Wall Human Pose Estimation Using Radio Signals](http://rfpose.csail.mit.edu/)
2. [A list of awesome papers and cool resources on WiFi CSI sensing](https://github.com/Marsrocky/Awesome-WiFi-CSI-Sensing#awesome-wifi-sensing)
