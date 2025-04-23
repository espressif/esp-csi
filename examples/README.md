# Example Projects
[[中文]](./README_cn.md)  
This directory contains multiple example projects for esp-csi. These examples are intended to demonstrate various features of esp-csi and provide code that can be copied and adapted for your own projects.

- `get-started/csi_recv`: A basic CSI data reception example showing how to obtain CSI information via a Wi-Fi receiver.
- `get-started/csi_send`: A basic CSI transmission example designed to work with `csi_recv`, sending Wi-Fi packets for the receiver to extract CSI.
- `get-started/csi_recv_router`: Receives CSI in router communication mode by pinging a router and parsing the CSI from its reply packets.
- `esp-radar/connect_rainmaker`: Connects CSI data to Espressif's Rainmaker cloud platform for remote visualization or control.
- `esp-radar/console_test`: A console-based testing example for debugging and evaluating CSI data and algorithm performance.
- `esp-crab/master_recv`: The master receiver for the esp-crab hardware platform; responsible for acquiring and parsing Wi-Fi CIR/CSI data.
- `esp-crab/slave_recv`: The slave receiver on the esp-crab platform, assisting the master receiver with multi-channel data collection.
- `esp-crab/slave_send`: The transmitter on the esp-crab platform, responsible for sending packets periodically for CSI extraction by other nodes.
