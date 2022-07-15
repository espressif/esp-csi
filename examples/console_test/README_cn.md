# esp-csi console_test [[English]](./README.md)
---
## 1 简介
console_test 是一款 Wi-Fi CSI 的测试平台，它包含了对 Wi-Fi CSI 的数据采集、数据显示、数据分析等功能，帮助我们更好的了解和应用 Wi-Fi CSI
+ **显示**：通过查看 Wi-Fi 射频底噪、CSI、RSSI 及射频底噪等实时数据，可以快速了解不同天线、人体运动和设备摆放对 Wi-Fi 信号的影响。
+ **采集**：所有采集到的 Wi-Fi CSI 将存储在文件中。您还可以采集不同运动行为的数据，以供以后的神经网络和机器学习使用
+ **分析**：可实现检测人体是否运动及房间内是否有人，帮助您快速了解 Wi-Fi CSI 的应用场景
  
## 2 环境准备
### 2.1 设备
![在这里插入图片描述](https://img-blog.csdnimg.cn/3a5aac67f46e42f99d65a339c0422cb6.png)
本例提供了 esp32-s3 开发板和路由器两种设备模式作为 Wi-Fi CSI 发送设备。 其中使用 esp32-s3 开发板作为发送设备，对发送频率、射频大小、信道的调整效果更好。两种模式下都用 esp32-s3 作为 Wi-Fi CSI 的接收设备。

### 2.2 编译环境
使用的 esp-idf 版本为 [ESP-IDF Release v4.4.1](https://github.com/espressif/esp-idf/releases/tag/v4.4.1)
```bash
cd esp-idf
git checkout v4.4.1
git submodule update --init --recursive
./install.sh
. ./export.sh
```

## 3 程序启动
### 3.1 发送 Wi-Fi CSI
+ **esp32-s3 发送 CSI**：烧录工程 `csi_send` 到 esp32-s3 中
    ```bash
    cd esp-csi/examples/get-started/csi_send
    idf.py set-target esp32s3
    idf.py flash -b 921600 -p /dev/ttyUSB0 monitor
    ```
+ **路由器 发送 CSI**：此时路由器不连接其他智能设备，避免网络拥塞影响测试效果

### 3.2 接收 Wi-Fi CSI
+ 烧录 `console_test` 到另一块 esp32-s3 开发板中
    ```bash
    cd esp-csi/examples/console_test
    idf.py set-target esp32s3
    idf.py flash -b 921600 -p /dev/ttyUSB1
    ```

### 3.3 启动 `esp-csi-tool` 工具，打开 CSI 实时可视化工具
+ 运行 `csi_recv` 中的 `esp_csi_tool.py` 进行数据分析，运行前请关闭 `idf.py` 监控
    ```bash
    cd esp-csi/examples/console_test/tools
    # Install python related dependencies
    pip install -r requirements.txt
    # Graphical display
    python esp_csi_tool.py -p /dev/ttyUSB1
    ```
+ 运行成功后，打开如下 CSI 数据实时可视化界面，界面左侧为数据显示界面，右侧为数据模型界面：
![在这里插入图片描述](https://img-blog.csdnimg.cn/aa20b80a379743d0adfd789e6b300358.png)

## 4 界面介绍
实时可视化界面由数据显示界面 `Raw data` 和数据模型界面 `Radar model` 两部分组成，`Raw data` 显示原始的 CSI 各个子载波的数据，`Radar model` 是使用算法对 CSI 数据分析后的结果，可以用于有人/无人、运动/静止的检测，通过选择勾选右上角 `Raw data` 和 `Radar model` 按钮，可以选择单独显示两个界面

### 4.1 路由器连接窗口：
左侧最上方为配置路由器连接信息窗口，使用设备连接到路由器上，接收路由器的与设备之间 CSI
+ **ssid**：路由器帐号
+ **password**：路由器密码
+ **auto connect**：如果勾选，下次打开时，将自动连接上次连接的路由器
+ **connect / disconnect**：连接/断开按钮
+ **custom**：可以发送更多的控制指令如：重启、版本获取，或者输入设备端自定义的命令
![在这里插入图片描述](https://img-blog.csdnimg.cn/ef825cae20bc4e0881703a42db7b7d71.png#pic_center)

### 4.2 CSI 数据波形显示窗口
此窗口可实时显示部分通道 CSI 数据的波形，如果勾选 `wave filtering`，显示的就是滤波后的波形
![在这里插入图片描述](https://img-blog.csdnimg.cn/60e9e76ee5764a9897f0335cde0b9c66.png#pic_center)

### 4.3 RSSI 波形显示窗口
此窗口显示 RSSI 波形信息，可用于与 CSI 波形比较，观测室内人员处于不同处态时 RSSI 变化情况
![在这里插入图片描述](https://img-blog.csdnimg.cn/b2bcfcc2997b4b2fa35548c805a83372.png#pic_center)

### 4.4 log 显示窗口
此窗口显示时间，内存等系统日志
![在这里插入图片描述](https://img-blog.csdnimg.cn/ddfe9ff87622448193f549140c561ab4.png#pic_cente)

### 4.5 Wi-Fi 信道数据显示窗口
此窗口显示 Wi-Fi 信道状态信息
![在这里插入图片描述](https://img-blog.csdnimg.cn/2580c161ed904541860f7a0c91734b29.png#pic_center)

### 4.6 室内状态显示窗口
此窗口用于校准室内状态阈值，和显示室内状态（有人/无人、运动/静止）
![在这里插入图片描述](https://img-blog.csdnimg.cn/be0e50367a0b4fd8a739c32328f8cda5.png#pic_center)

+ **delay**：开始校准延迟时间，校准时要求室内无人，人员可在开始校准后，延迟时间内离开房间
+ **duration**：校准过程持续时间
+ **add**：如果勾选，再次校准后的阈值将在当前阈值基础上累加
+ **start**：开始校准按钮
+ **wander(someone) threshold**：判断室内有人/无人的阈值，将在校准后自动设置，也可以用户自己手动设置
+ **jitter(move) threshold**：判断人员静止/运动的阈值，将在校准后自动设置，也可以用户自己手动设置
+ **config**：用户手动设置阈值后，点击配置按钮
+ **display table**：如果勾选，将在波形框右侧显示室内状态和人员状态信息表格，表格中具体参数如下
    |status|threshold|value|max|min|mean|std|
    |---|---|---|---|---|---|---|
    |状态|阈值|实时值|最大值|最小值|平均值|标准差|

### 4.7 人员运动数据显示窗口
此窗口显示室内人员运动的具体数据，左侧以柱状图实时显示人员运动次数，右侧表格记录具体运动时间
![在这里插入图片描述](https://img-blog.csdnimg.cn/1f98b665229748fca0bc0c1097f35ee6.png#pic_center)

+ **mode**：观测模式，`minute, hour, day` 三种模式分别为以每分钟、每小时、每天为单位记录人员运动的次数
+ **time**：时间，默认当前时间，可手动设置开始观测的时间
+ **auto update**：如果勾选，会自动实时更新显示人员运动次数柱状图
+ **update**：点击后会手动更新显示人员运动次数柱状图

表格各参数意义如下：
|room|human|spend_time|start_time|stop_time|
|---|---|---|---|---|
|房间状态|人员状态|运动时长|运动开始时间|运动结束时间|

### 4.8 动作采集窗口
此窗口用于对人员做不同动作时的 CSI 数据进行采集，采集的数据将存储在 `esp-csi/examples/console_test/tools/data` 路径下，采集的数据可用于机器学习或神经网络
![在这里插入图片描述](https://img-blog.csdnimg.cn/66679e02b14f4d7a9ac3ec737432509e.png#pic_center)
+ **target**：选择要采集的动作
+ **delay**：选择采集的延迟时间，即点击开始按钮后延迟多久开始采集
+ **duration**：采集一次动作持续时长
+ **number**：采集次数
+ **clean**：点击后将清除所采集的所有数据
+ **start**：开始采集按钮

### 4.9 模型评估窗口
此窗口用于对所采用的阈值的优劣性进行评估，根据发送的采样结果数据，对室内状态、人员状态检测结果进行准确率评估
![在这里插入图片描述](https://img-blog.csdnimg.cn/1fd84ab5d39b4db2b7979e30cde02161.png#pic_center)
+ **open folder**：打开采样结果的数据文件
+ **send**：发送文件，发送后模型将对动作进行识别，并评估识别准确率

## 5 操作流程
这里以连接路由器为例，介绍了可视化系界面使用的操作流程
### 5.1 连接路由器 
+ 在路由器连接窗口中依次输入路由器帐号和密码 
+ （可选）勾选 `auto connect`
+ 点击 `connect`

连接成功后，将在 “log 打印窗口” 看到路由器状态信息，同时 “CSI 数据波形显示窗口” 显示 CSI 数据波形

### 5.2 校准阈值
校准的目的是为让设备知晓房间无人状态下的 CSI 信息，并获取到人员移动的阈值，当前版本如果不校准仅能进行人员移动检测，校准时间越长误触的概率越低
![在这里插入图片描述](https://img-blog.csdnimg.cn/d53d535927a140efa1d1716e8624e500.png#pic_center)
+ 设置 `delay` 延迟时间，这里设置 10 秒为例（即 00:00:10），以便人员可以离开房间
+ 设置 `duration` 校准持续时长， 这里设置 10 秒为例
+ 点击 `start` 后选择 `yes`，人员 10 秒（`delay` 延迟时间）内离开房间，确保校准期间 10 秒内（`duration` 校准持续时长）房间内无人，校准结束后返回房间
+ （选择）可根据校准结果，手动调整房间状态阈值和人员状态阈值

### 5.3 观测房间状态和人员状态
校准结束后，室内状态显示窗口中将显示室内状态，并判断房间内无人、有人静止、有人运动三种状态
![在这里插入图片描述](https://img-blog.csdnimg.cn/99c107f9d9e74150a3b93f6fb3408f73.png#pic_center)
+ 在 `filter outliers` 中设置连续多少次内达到阈值多少次判定为有人/运动状态，以便过滤异常值
+ 点击 `config` 进行配置
+ （选择）在房间状态波形窗口，点击 `wander` 前的横线，可隐藏其波形，方便观测其他波形，同样其他波形也可以用次方法隐藏
+ （选择）勾选 `display table` ,查看室内状态和人员状态信息表格

### 5.4 观测人员运动次数和时间
在些观测窗口中，将会按照我们的设置以柱状图显示人员每分钟的运动数量在右侧表格中记录了每次运动时间信息。
![在这里插入图片描述](https://img-blog.csdnimg.cn/c6bb4d5518c941cea3d3f53b1daccbc9.png#pic_center)

+ 在人员运动数据显示窗口的 `mode` 中，选择观测模式，可以查看某分钟、某小时或某天房间内人员的运行情况，这里选择 `minute` 为例
+ （选择）在 `time` 中设置开始观测的时间，默认为当前时刻开始
+ 勾选 `auto update` 自动更新检测结果，如果不勾选，每次点击 `update` 会更新一次检测结果

### 5.5 采集特定动作的 CSI 数据
![在这里插入图片描述](https://img-blog.csdnimg.cn/cdbeef937dc3456db3bf51097481a639.png#pic_center)
+ （选择）在动作采集窗口的 `clean` 中清除以前的采集数据记录
+ 在 `target` 中选择将要采集的动作，这里选择 `move` 为例
+ 在 `delay` 中设置点击开始后的延迟采集时间，这里设置延迟 5 秒为例
+ 在 `duration` 中设置采集一个动作持续的时间，这里选择 500 ms 为例
+ 在 `number` 中设置要采集的次数，这里采集 1 次为例
+ 点击 `start`，经过延迟时间后系统将开始采集数据，人员在设置时间内完成对应动作 

采集结束后会自动停止，我们在 `esp-csi/examples/console_test/tools/data` 路径下可以看到我们采集的数据
![在这里插入图片描述](https://img-blog.csdnimg.cn/102d7ec49af5467f8d72a6d5e6d69e33.png#pic_center)

### 5.6 利用采集的数据识别动作，并评估准确率
采集实时 CSI 信息，并通过发送的数据对动作进行实时识别，并显示准确率，以此来评估设置的阈值优劣性，如果准确率较低，可再次调整阈值
![在这里插入图片描述](https://img-blog.csdnimg.cn/61e6174db21b4714bb7f594c786cd4e9.png#pic_center)![在这里插入图片描述](https://img-blog.csdnimg.cn/06fa256d26294cf6bf519b1ca6961d2e.png#pic_center)
+ 点击 `open folder`，选择采集的数据文件
+ 点击 `send` 后选择 `yes`
+ 点击 `csi start`

### 5.7 窗口放大与缩小
![在这里插入图片描述](https://img-blog.csdnimg.cn/f760ef7700e04eeab3392d7da787ca10.png#pic_center)
+ 通过选择勾选界面右上角 `Raw data` 与 `Radar model`，可单独显示 “数据显示界面” 和 “数据模型界面”
+ 鼠标选中不同窗口间的临界线，通过拖拽可放大/缩小各窗口
