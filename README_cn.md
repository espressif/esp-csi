# ESP-CSI [[English]](./README.md)

本项目的主要目的是展示 ESP-WIFI-CSI 的使用。本项目提供了 CSI 数据的获取方法、处理算法和应用案例。人体检测算法仍在优化中。基于原始 CSI 数据，用户可利用机器学习、神经网络等算法来得到更精确的结果。

## CSI 介绍

信道状态信息（CSI, Channel State Information）是描述无线信道特性的重要参数，包括信号的幅度、相位、信号延迟等指标。在 Wi-Fi 通信中，CSI 用于测量无线网络的信道状态。通过分析和研究 CSI 的变化，可以推断引起信道状态变化的物理环境变化，实现非接触式智能传感。CSI 对环境变化非常敏感。它不仅能感知人或动物的行走、奔跑等大动作引起的变化，还能捕捉静态环境中人或动物的细微动作，如呼吸、咀嚼等。这些能力使得  CSI 在智能环境监测、人体活动监测、无线定位等应用中具有广泛的应用前景。

## 基础知识

为了更好地理解 CSI 技术，我们提供了一些相关的基础知识文档（近期更新会逐步更新）：

- [信号处理基础](./docs/zh_CN/Signal-Processing-Fundamentals.md)
- [OFDM介绍](./docs/zh_CN/OFDM-introduction.md)
- [无线信道基础](./docs/docs/zh_CN/Wireless-Channel-Fundamentals.md)
- [无线测距与定位技术介绍](./docs/zh_CN/Introduction-to-Wireless-Location.md)
- [无线通信指标CSI与RSSI](./docs/zh_CN/Wireless-indicators-CSI-and-RSSI.md)
- [CSI的应用与案例分析](./docs/zh_CN/CSI-Applications.md)

## Espressif CSI 优势

- **全系列支持:** 所有的 ESP32 系列均支持 CSI，ESP32 / ESP32-S2 / ESP32-C3 / ESP32-S3 / ESP32-C6
- **强大的生态:** Espressif 是 Wi-Fi MCU 领域的全球领导者，将 CSI 与现有物联网设备完美结合
- **更多信息:** ESP32 提供丰富的信道信息，包括 RSSI、RF 噪声本底、接收时间和天线的 'rx_ctrl' 字段
- **蓝牙辅助:** ESP32 也支持 BLE，例如，它可以扫描周围的设备来辅助检测
- **强大的处理能力:** ESP32 的 CPU 是双核 240MHz，支持 AI 指令集，能够运行机器学习和神经网络
- **OTA 升级:** 现有项目可通过软件 OTA 升级 CSI 新功能，无需增加额外的硬件成本

## 例程介绍

### [get-started](./examples/get-started)

帮助用户快速上手 CSI 功能，通过基础示例展示 CSI 数据的获取与初步分析，详情查看 [README](./examples/get-started/README.md)

- [csi_recv](./examples/get-started/csi_recv) 演示了 ESP32 作为接收端示例
- [csi_send](./examples/get-started/csi_send) 演示了 ESP32 作为发送端示例
- [csi_recv_router](./examples/get-started/csi_recv_router) 演示了路由器作为发送端示例，ESP32 通过 Ping 触发路由器发送 CSI 报文
- [tools](./examples/get-started/tools) 提供辅助 CSI 数据分析的脚本 csi_data_read_parse.py

### [esp-radar](./examples/esp-radar)

提供了利用 CSI 数据实现的一些应用，包括 RainMaker 云端上报和人体活动检测

- [connect_rainmaker](./examples/esp-radar/connect_rainmaker) 演示了将 CSI 数据捕获并上传到 Espressif 的 RainMaker 云平台
- [console_test](./examples/esp-radar/console_test) 演示一个交互式控制台，允许动态配置和捕获 CSI 数据，并提供了人体活动检测的算法应用

## 如何获取CSI

### 获取路由器 CSI

<img src="docs/_static/get_router_csi.png" width="550">

- **实现方法：** ESP32 向路由器发送 Ping 包，并接收路由器返回的 Ping 回应中的 CSI 信息。
- **优点：** 只需一个 ESP32 和路由器即可完成。
- **缺点：** 依赖于路由器的条件，如路由器的位置、支持的 Wi-Fi 协议等。
- **适用场景：** 环境中只有一个 ESP32，并且检测环境中有路由器。

### 获取设备之间的 CSI

<img src="docs/_static/get_device_csi.png" width="550">

- **实现方法：** ESP32 A 和 B 都向路由器发送 Ping 包，ESP32 A 接收 ESP32 B 发出的 Ping 中的 CSI 信息，这是对第一种检测场景的补充
- **优点：** 不依赖于路由器的位置，也不受其他连接到路由器设备的影响
- **缺点：** 依赖于路由器支持的 Wi-Fi 协议和环境
- **适用场景：** 环境中必须有两个或以上的 ESP32

### 获取特定设备的 CSI

<img src="docs/_static/get_broadcast_csi.png" width="550">

- **实现方法：** 数据包发送设备不断切换信道发送数据包，ESP32 A、B 和 C 都获取数据包发送设备广播数据包中的 CSI 信息，这种方法的检测精度和可靠性最高
- **优点：** 不受路由器影响，检测精度高。当环境中有多个设备时，只有一个数据包发送设备会对网络环境造成很小的干扰
- **缺点：** 除了普通的 ESP32，还需要额外的专用数据包发送设备，成本相对较高
- **适用场景：** 适用于需要高精度和多设备集群定位的场景

## 注意事项

1. 外置 IPEX 天线效果优于 PCB 天线，PCB 天线具有方向性。
2. 测试应在无人环境中进行，避免他人活动对测试结果的影响。

## 相关资源

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html) is the documentation for the Espressif IoT development framework.
- [ESP-WIFI-CSI Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-channel-state-information) is the use of ESP-WIFI-CSI Description.
- 如果您发现 BUG 或有 feature 请求, 可以在 GitHub [Issues](https://github.com/espressif/esp-csi/issues)上提交。在提交之前，请检查您的问题是否已存在于现有问题中

## Reference

1. [Through-Wall Human Pose Estimation Using Radio Signals](http://rfpose.csail.mit.edu/)
2. [A list of awesome papers and cool resources on WiFi CSI sensing](https://github.com/Marsrocky/Awesome-WiFi-CSI-Sensing#awesome-wifi-sensing)
