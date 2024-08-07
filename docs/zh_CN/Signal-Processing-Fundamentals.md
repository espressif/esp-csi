# 无线通信系统的组成[[English]](./docs/en/Signal-Processing-Fundamentals.md)

发射器和接收器：信号的生成、传输和接收设备。
信道：传输信号的媒介，可以是自由空间、空气、或其他介质。

## 信号的采样与量化

采样：将连续信号转换为离散信号的过程。
量化：将离散信号的幅度值转换为有限精度的过程。
奈奎斯特采样定理：采样率必须至少是信号最高频率的两倍。

## 傅里叶变换

离散傅里叶变换（DFT）：将离散时间信号转换为频域表示。
快速傅里叶变换（FFT）：高效计算DFT的算法。

## 调制与解调技术

调制：将基带信号转换为载波信号的过程。
解调：从载波信号中恢复基带信号的过程。
常见调制技术：如QAM（正交振幅调制）、PSK（相移键控）、OFDM（正交频分复用）。

## CSI 子载波的利用

CSI通过子载波获取信道信息的能力源于正交频分复用（Orthogonal Frequency Division Multiplexing，OFDM）和正交频分复用多输入多输出（Orthogonal Frequency Division Multiplexing Multiple Input Multiple Output，OFDM-MIMO）等技术。
OFDM技术将整个频谱划分为多个相互正交的子载波，每个子载波之间相互独立传输数据。这些子载波在频率域上彼此正交，这意味着它们之间的干扰较小。通过在不同的子载波上发送数据，OFDM技术能够提高频谱利用率和抗干扰能力。
在OFDM-MIMO系统中，多个天线同时发送不同的数据流，这些数据流经过信道传输到接收端，并受到多径传播、衰落等信道效应的影响。由于信道会对不同的子载波产生不同的影响，因此可以利用这些子载波上的信号特性来获取信道信息。通过观察发送的信号与接收的信号之间的差异，可以推断出信道的状态信息，包括信道的衰落、相位偏移等。
因此，CSI能够通过子载波获取信道信息是基于OFDM技术和MIMO技术的特性，利用多个子载波和多个天线之间的信号差异来推断信道状态信息。
