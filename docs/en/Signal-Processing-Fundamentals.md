# Components of a Wireless Communication System [[中文]](docs/zh_CN/Wireless-Channel-Fundamentals.md)

**Transmitter and Receiver**: Devices for generating, transmitting, and receiving signals.
**Channel**: The medium through which signals are transmitted, which can be free space, air, or other media.

## Signal Sampling and Quantization

**Sampling**: The process of converting a continuous signal into a discrete signal.
**Quantization**: The process of converting the amplitude values of a discrete signal to finite precision.
**Nyquist Sampling Theorem**: The sampling rate must be at least twice the highest frequency of the signal.

## Fourier Transform

**Discrete Fourier Transform (DFT)**: Converts a discrete-time signal into its frequency domain representation.
**Fast Fourier Transform (FFT)**: An algorithm for efficiently computing the DFT.

## Modulation and Demodulation Techniques

**Modulation**: The process of converting a baseband signal into a carrier signal.
**Demodulation**: The process of recovering the baseband signal from the carrier signal.
**Common Modulation Techniques**: Such as Quadrature Amplitude Modulation (QAM), Phase Shift Keying (PSK), and Orthogonal Frequency Division Multiplexing (OFDM).

## Utilization of CSI Subcarriers

The ability of CSI to obtain channel information through subcarriers is derived from technologies such as Orthogonal Frequency Division Multiplexing (OFDM) and Orthogonal Frequency Division Multiplexing Multiple Input Multiple Output (OFDM-MIMO).
OFDM technology divides the entire spectrum into multiple orthogonal subcarriers, each of which transmits data independently. These subcarriers are orthogonal in the frequency domain, which means they have minimal interference with each other. By transmitting data on different subcarriers, OFDM technology can improve spectral efficiency and resistance to interference.
In an OFDM-MIMO system, multiple antennas simultaneously transmit different data streams. These data streams are transmitted through the channel to the receiving end and are affected by channel effects such as multipath propagation and fading. Since the channel affects different subcarriers differently, the characteristics of signals on these subcarriers can be used to obtain channel information. By observing the differences between transmitted and received signals, one can infer channel state information, including channel fading, phase shifts, etc.
Therefore, the ability of CSI to obtain channel information through subcarriers is based on the characteristics of OFDM and MIMO technologies, utilizing the signal differences between multiple subcarriers and multiple antennas to infer channel state information.