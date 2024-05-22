# ESP RainMaker 中添加 Wi-Fi CSI 功能 [[English]](./README.md)

## 编译下载
按照 ESP RainMaker 文档 [入门](https://rainmaker.espressif.com/docs/get-started.html) 部分构建和烧录固件

## 如何使用这个例子
### 参数说明
- **someone_status**: false - 无人, true - 有人
- **someone_timeout**: 房间内在此时间内有人移动则标识为有人，单位为秒
- **move_status**: false - 无人移动, true - 有人移动
- **move_count**: 与上一次上报 `ESP RainMaker` 之间检测到的有人移动的次数
- **move_threshold**: 用于判断是否有人在移动的阈值
- **filter_window**: `Wi-Fi CSI` 的波形的抖动值的缓冲队列的大小，用于过滤异常值
- **filter_count**: 在波形的抖动值的缓冲队列内，当检测到 `filter_count` 次，`Wi-Fi CSI` 的波形的抖动值大于 `move_threshold`，标记为有人移动
- **threshold_calibrate**: 是否使能自校准
- **threshold_calibrate_timeout**: 自校准超时时间，单位为秒

### App 版本
- ESP RainMaker App: [v2.11.1](https://m.apkpure.com/p/com.espressif.rainmaker)+
> 注： `ESP RainMaker` App 2.11.1 版本之前的版本不支持 `time series` 功能，`move_count` 无法正常显示

### APP 操作
1. 打开 RainMaker App
2. 点击 `+` 添加设备
3. 等待设备连接到云端
4. 点击 `threshold_calibrate` 使能自校准，校准时需要保证房间内无人或人不移动
5. 校准完成后，有人移动的阈值会显示在 `move_threshold` 中，此时检测到有人移动 move_status 会变为 true

### 设备
- [x] ESP32-S3-DevKitC-1
- [x] ESP32-C3-DevKitC

### 设备操作
1. **恢复出厂设置**：按住 `BOOT` 按钮 5 秒以上可将开发板重置为出厂默认设置

## 设备状态
- 人体移动检测
    - 房间内有人移动: LED 亮绿灯
    - 房间内无人移动: LED 亮白灯
 
- 人体存在检测
    > 当前算法人体存在检测效果不理想，因此判断是否有人移动的依据是：1 分钟内是否有人移动，如果有人移动则认为有人存在，否则认为无人存在
    - 房间内有人: LED 亮起
    - 房间内无人: LED 熄灭

- 人体移动检测阈值
    > - 人体移动检测阈值可以通过手机 App 设置也可以通过自校准获取，如果没有设置则使用默认值
    > - 校准时需要保证房间内无人或人不移动，校准后检测灵敏度会提高，但是如果房间内有人移动则会导致误检测，因此建议在房间内无人时进行校准
    > - 校准后将会保存在 NVS 中，下次重启后会使用保存的阈值
    - 进行人体移动阈值校准时，LED 黄色闪烁

## 常见问题

### RainMaker 上报失败

------
- **问题**: 设备端日志上打印如下错误
    ```shell
    E (399431) esp_rmaker_mqtt: Out of MQTT Budget. Dropping publish message.
    ```
- **原因**: 设备端上报的数据量超过了 `ESP RainMaker` 的限制

------
- **问题**: 一直检测到有人移动但是实际上没有人移动，或者设备端一直检测不到人移动

- **解决方式**:
    1. 人体移动检测阈值不对导致误识别
        - Wi-Fi CSI 默认人体移动检测阈值无法满足实际需求，需要根据实际情况调整或使能自校准，可以通过手机 App 设置
        - Wi-Fi CSI 默认的离群点过滤窗口大小无法满足实际需求，需要根据实际情况调整，可以通过手机 App 设置

    2. 网络不稳定导致检测不准确
       - 更换路由器后是否能正常工作
       - 是否可以尝试将路由器放置在更合适的位置
  
    3. 以上方式仍旧无法解决，修改 LOG 等级，在 github 上提交 [issue](https://github.com/espressif/esp-csi/issues)
        ```c
        esp_log_level_set("esp_radar", ESP_LOG_DEBUG);
        ```
