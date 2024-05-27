# Introduction to OFDM[[中文]](docs/zh_CN/OFDM-introduction.md)

To understand the principles of CSI, one must first grasp the basics of the physical layer in Wi-Fi transmission. The "O" in OFDM stands for "Orthogonal," so let's start with the definition of orthogonality.

## Definition of Orthogonality

In mathematics, two functions (or vectors) are called orthogonal if their inner product is zero over a given interval. More specifically, for two functions $f(x)$ and $g(x)$ over the interval $[a, b]$, their inner product is given by the integral:
$\langle f, g \rangle = \int_{a}^{b} f(x) \cdot g(x) \, dx$

If this integral is zero, then $f(x)$ and $g(x)$ are orthogonal over the interval $[a, b]$.

## Simple Orthogonal Model

Let's start with the simplest model: for any $\omega_1 \ne \omega_2$, $\sin(\omega_1 t)$ and $\sin(\omega_2 t)$ are orthogonal over one period.

To prove their orthogonality over one period, consider the inner product of these two sine functions over one period $[0, T]$:
$\int_{0}^{T} \sin(\omega_1 t) \sin(\omega_2 t) \, dt$

Using trigonometric identities, we can simplify this integral. Using the identity:
$$\sin(A) \sin(B) = \frac{1}{2}[\cos(A-B) - \cos(A+B)]$$
we can expand the integral into the difference of two cosine functions:
$$\int_{0}^{T} \sin(\omega_1 t) \sin(\omega_2 t) \, dt = \frac{1}{2} \int_{0}^{T} [\cos((\omega_1 - \omega_2) t) - \cos((\omega_1 + \omega_2) t)] \, dt$$

Since $\omega_1$ and $\omega_2$ are different frequencies, both $(\omega_1 - \omega_2)$ and $(\omega_1 + \omega_2)$ are non-zero. Therefore, the integrals of these cosine functions over one period are zero. This means that $\sin(\omega_1 t)$ and $\sin(\omega_2 t)$ have an inner product of zero over one period, i.e., they are orthogonal over one period.

## Time-Domain OFDM

In a simple model, $\sin(\omega_1 t)$ and $\sin(\omega_2 t)$ are orthogonal. Over the duration $[0, T]$, the easiest way to transmit signals is through amplitude modulation: $\sin(\omega_1 t)$ transmits signal $a$, thus sending $a \cdot \sin(\omega_1 t)$, and $\sin(\omega_2 t)$ transmits signal $b$, thus sending $b \cdot \sin(\omega_2 t)$. The sine waves $\sin(\omega_1 t)$ and $\sin(\omega_2 t)$ serve as carriers and are predetermined information between the transmitter and receiver, called subcarriers. **The amplitude signals $a$ and $b$ modulated onto the subcarriers are the actual information to be transmitted.** The signal transmitted through the channel is $a \cdot \sin(\omega_1 t) + b \cdot \sin(\omega_2 t)$. At the receiver end, the signals are detected by integrating the received signal with respect to $\sin(\omega_1 t)$ and $\sin(\omega_2 t)$, thus retrieving $a$ and $b$. Below is the mathematical explanation of orthogonality applied to OFDM:

When receiving the signal, orthogonality allows different frequency signals to be decoded independently. Specifically, the transmitted signal $s(t)$ consists of two sub-signals:
$a \sin(\omega_1 t) + b \sin(\omega_2 t)$
where $a$ and $b$ are the amplitudes, and $\omega_1$ and $\omega_2$ are the frequencies of the two sub-signals.

Now, let's decode one of the sub-signals, $a$.

After receiving the signal, the receiver multiplies it by the demodulation frequency $\sin(\omega_1 t)$ and integrates, aiming to decode the sub-signal $a \sin(\omega_1 t)$.

Due to orthogonality, $\sin(\omega_1 t)$ and $\sin(\omega_2 t)$ have an inner product of zero over one period:
$\int_{0}^{T} \sin(\omega_1 t) \sin(\omega_2 t) \, dt = 0$

Thus, during integration, the term $\sin(\omega_1 t) \sin(\omega_2 t)$ disappears, leaving only the sub-signal $a \sin^2(\omega_1 t)$.

We know that $\sin^2(\omega_1 t)$ integrates to half its period over one period:
$\int_{0}^{T} \sin^2(\omega_1 t) \, dt = \frac{T}{2}$

Therefore, the decoded signal $a$ is:
$a \times \frac{T}{2}$

Using this method, the receiver can independently decode each sub-signal $a$ and $b$ because orthogonality ensures that other sub-signals do not contribute to the integral.

Taking $\sin(\omega_1 t)$ and $\sin(\omega_2 t)$ as examples, we demonstrate the transition from intuitive to abstract. The simple orthogonal model is the foundation of all complex theories.

Next, expand the model of $\sin(t)$ and $\sin(2t)$ to more subcarrier sequences $\{ \sin(2T \Delta f \cdot t), \sin(2T \Delta f \cdot 2t), \sin(2T \Delta f \cdot 3t), \ldots, \sin(2T \Delta f \cdot kt) \} $ (e.g., $k=16, 256, 1024$), where $2\pi$ is a constant, $\Delta f $ is the preselected carrier frequency interval, $T$ is the period, and $k$ is the maximum index of the sequence.

The orthogonality of multiple functions is based on their pairwise orthogonality. If a set of functions $\{f_1(t), f_2(t), ..., f_n(t)\}$ is pairwise orthogonal over an interval, then for any $i \ne j$:
$\int_{a}^{b} f_{i}(t) f_{j}(t) dt = 0$
This means that every pair of different functions $f_i(t)$ and $f_j(t)$ have an inner product of zero. The orthogonality of the above subcarrier sequence can be easily proven by $\Delta f \ne 0$.

The impact of frequency spacing: if $\Delta f$ is chosen small enough such that $2T \Delta f \cdot k$ covers the entire spectrum, this ensures that all sine waves do not overlap in frequency. Therefore, each sine wave has a zero integral over one complete period, ensuring orthogonality.

In the next step, we introduce $\cos(t)$. It's easy to prove that $\cos(t)$ is orthogonal to $\sin(t)$ and to the entire orthogonal family of $\sin(kt)$. Similarly, $\cos(kt)$ is orthogonal to the entire orthogonal family of $\sin(kt)$.

Thus, the sequence model extends to $\{\sin(2T \Delta f t), \sin(2T \Delta f \cdot 2t), \ldots, \sin(2T \Delta f kt), \cos(2T \Delta f t), \cos(2T \Delta f \cdot 2t), \ldots, \cos(2T \Delta f kt)\}$.

After expanding the orthogonal sequences $\sin(kt)$ and $\cos(kt)$, these are just the transmission "mediums." The actual information to be transmitted still needs to be modulated onto these carriers, i.e., $\sin(t), \sin(2t), \ldots, \sin(kt)$ are amplitude modulated with signals $a_1, a_2, \ldots, a_k$, and $\cos(t), \cos(2t), \ldots, \cos(kt)$ are amplitude modulated with signals $b_1, b_2, \ldots, b_k$. What kind of waveform is formed when these 2n orthogonal signals are transmitted simultaneously? The signal $f(t)$ can be expressed as follows (Equation 1-1):
$f(t)=a_1 \sin(\Delta f \cdot t)+a_2 \sin(\Delta f \cdot 2t)+ \cdots + a_k \sin(\Delta f \cdot kt)+ b_1 \cos(\Delta f \cdot t)+b_2 \cos(\Delta f \cdot 2t)+ \cdots + b_k \cos(\Delta f \cdot kt)$

Because orthogonal signals separate sub-signals of different frequencies, at the receiving end, through Fourier transform, we can separately obtain these amplitude signals $a_i$ and $b_i$.

Considering that in modern signal processing, both time and frequency domains are discretized, the formula (1-1) becomes:

$s[n]= \sum_{k=0}^{N-1} x[k] e^{j2\pi nk/N}$

where $x[k]$ are the amplitude signals of sub-signals, and $N$ is the number of Fourier transform points.

Therefore, the principle of OFDM is to decompose the time signal into different frequency sub-signals through Fourier transform, achieve parallel transmission through orthogonality, and then recover the original signal at the receiving end through inverse Fourier transform. This greatly improves spectral efficiency and interference resistance.

## Application of OFDM in Wireless Communication

OFDM is widely used in wireless communications, especially in Wi-Fi, 4G, and 5G mobile communications. Its main advantages include:

1. **Resistance to Multipath Interference**: Since OFDM decomposes the signal into multiple sub-signals transmitted on different frequencies, multipath interference only affects part of the sub-signals. The receiver can use equalization algorithms to reduce interference.
2. **High Spectral Efficiency**: The orthogonality of OFDM ensures that sub-signals can overlap in the spectrum without causing interference, improving spectral efficiency.
3. **Flexible Subcarrier Allocation**: OFDM can dynamically allocate subcarriers based on channel conditions, enhancing transmission flexibility and efficiency.
In summary, OFDM uses orthogonal frequency-division multiplexing technology to significantly improve spectral efficiency and interference resistance, making it a fundamental technology in modern wireless communication.
