# 示例程序
[[English]](./README.md)  
本目录包含多个 esp-csi 示例项目。这些示例旨在演示  esp-csi 的部分功能，并提供可以复制和修改以用于自己项目的代码。

- `get-started/csi_recv`：基本的 CSI 数据接收示例，展示如何通过 Wi-Fi 接收端获取 CSI 信息。
- `get-started/csi_send`：基本的 CSI 发送端示例，用于配合 `csi_recv` 发送 Wi-Fi 包以供接收侧获取 CSI。
- `get-started/csi_recv_router`：在路由器通信模式下接收 CSI，通过 ping 路由器并解析其应答包中的 CSI 信息。
- `esp-radar/connect_rainmaker`：将 CSI 数据与 Espressif 的云平台 Rainmaker 连接，用于远程展示或控制。
- `esp-radar/console_test`：用于控制台调试测试 CSI 数据和算法效果的示例。
- `esp-crab/master_recv`：esp-crab 硬件平台上的主接收端，支持获取并解析 Wi-Fi CIR/CSI 数据。
- `esp-crab/slave_recv`：esp-crab 平台的从接收端，辅助主接收端进行多通道数据收集。
- `esp-crab/slave_send`：esp-crab 平台的发送端，负责定时发送数据包供其他节点进行接收和 CSI 提取。