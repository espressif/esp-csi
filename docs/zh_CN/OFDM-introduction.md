# OFDM Introduction[[English]](./docs/en/OFDM-introduction.md)

要理解 CSI 的原理，首先要了解 Wi-Fi 传输的物理层基本知识。OFDM 的"O"代表着"正交"，那么先从正交的定义说起。

## 正交的定义

在数学中，两个函数（或向量）被称为正交，如果它们在给定区间上的内积为零。更具体地说，考虑两个函数 $f(x)$ 和 $g(x)$ 在区间 $[a, b]$ 上，它们的内积可以表示为积分：
$$\langle f, g \rangle = \int_{a}^{b} f(x) \cdot g(x) \, dx$$

如果这个积分结果为零，则称 $f(x)$ 和 $g(x)$ 在区间 $[a, b]$ 上是正交的。

## 简单正交模型

以最简单的模型为切入点：对于任意 $\omega_1 ≠ \omega_2$，有 $\sin(\omega_1 t)$与 $\sin(\omega_2 t)$ 在一个周期内的内积为零。

证明它们在一个周期内的正交性，对于这两个正弦函数，在一个周期 $[0, T]$ 上的内积可以表示为：$$\int_{0}^{T} \sin(\omega_1 t) \sin(\omega_2 t) \, dt$$

由于正弦函数的性质，这个积分可以通过使用三角恒等式进行简化。利用三角恒等式 
$$\sin(A) \sin(B) = \frac{1}{2}[\cos(A-B) - \cos(A+B)]$$
我们可以将上述积分展开为两个余弦函数的差：
$$\int_{0}^{T} \sin(\omega_1 t) \sin(\omega_2 t) \, dt = \frac{1}{2} \int_{0}^{T} [\cos((\omega_1 - \omega_2) t) - \cos((\omega_1 + \omega_2) t)] \, dt$$

由于 $\omega_1$ 和 $\omega_2$ 是不同的频率，$(\omega_1 - \omega_2)$ 和 $(\omega_1 + \omega_2)$ 都不为零。因此，这两个余弦函数在一个周期内的积分都为零。这意味着 $\sin(\omega_1 t)$ 与 $\sin(\omega_2 t)$ 在一个周期内的内积为零，即它们在一个周期内是正交的。

## 时域 OFDM

以直观情况举例，$sin(\omega_1 t)$ 和 $sin(\omega_2 t)$ 是正交的，在 $[0, T]$ 的时长内，采用最易懂的幅度调制方式传送信号：$sin(\omega_1 t)$ 传送信号 $a$，因此发送 $a·sin(\omega_1 t)$，$sin(\omega_2 t)$ 传送信号 $b$ 因此发送 $b·sin(\omega_2 t)$。其中，$sin(\omega_1 t)$  和 $ sin(\omega_2t)$  的用处是用来承载信号，是收发端预先规定好的信息，我们称为子载波；**调制在子载波上的幅度信号a和b，才是需要发送的信息**。因此在信道中传送的信号为 $a·sin(\omega_1 t) + b·sin(\omega_2 t)$。在接收端，分别对接收到的信号作关于 $sin(\omega_1 t)$ 和 $sin(\omega_2 t)$ 的积分检测，就可以得到 $a$ 和 $b$ 了。下面用公式解释正交应用于 OFDM ：

在接收信号时，正交性使得不同频率的信号可以独立解码。具体来说，传送信号 s(t) 是由两个子信号组成：$$a \sin(\omega_1 t) + b \sin(\omega_2 t)$$其中 a 和 b 是信号的振幅，$\omega_1$ 和 $\omega_2$ 分别是两个子信号的频率。

现在，我们来解码其中一个子信号 $𝑎$。

接收端接收到信号后，会乘以解调频率 $\sin(\omega_1 t)$ 并进行积分，这是因为我们想要解码子信号 $a \sin(\omega_1 t)$。

根据正交性，$\sin(\omega_1 t)$ 与 $\sin(\omega_2 t)$ 在一个周期内的内积为零：$$\int_{0}^{T} \sin(\omega_1 t) \sin(\omega_2 t) \, dt = 0$$

因此，接收端的积分过程中 $\sin(\omega_1 t) \sin(\omega_2 t)$ 项消失，只留下了子信号 $a \sin^2(\omega_1 t)$。

我们知道，$\sin^2(\omega_1 t)$ 在一个周期内的积分为其周期的一半：$$\int_{0}^{T} \sin^2(\omega_1 t) \, dt = \frac{T}{2}$$

因此，解码出的信号 a 为：$$a \times \frac{T}{2}$$
通过这种方法，接收端能够独立解码每个子信号 𝑎 和 𝑏，因为正交性确保了其他子信号对积分的贡献为零。

选取 \( \sin(\omega_1 t) \) 和 \( \sin(\omega_2 t) \) 作为例子，正是因为它们是介于直观和抽象的过渡地带。上面的正交模型虽然简单，但是却是所有复杂理论的基础。

下一步，将 $sin(\omega_1 t)$ 和 $sin(\omega_2 t)$ 的模型扩展到更多的子载波序列 $\{ \sin(2T \Delta f \cdot t), \sin(2T \Delta f \cdot 2t), \sin(2T \Delta f \cdot 3t), \ldots, \sin(2T \Delta f \cdot kt) \}$ （例如 $k=16, 256, 1024$ 等），其中，$2\pi$ 是常量，$ \Delta f$ 是事先选好的载频频率间隔，$T$ 是周期， $k$ 是序列的最大索引。

多个函数的正交性是基于它们两两之间的正交。如果一组函数 ${𝑓₁(𝑡), 𝑓₂(𝑡), ..., 𝑓ₙ(𝑡)}$ 在某个区间上成对正交，那么对于任意 𝑖≠𝑗，都有：
$\int_{a}^{b} 𝑓_{𝑖}(𝑡) 𝑓_{𝑗}(𝑡) \, dt = 0$
这意味着，每一对不同的函数 $𝑓_𝑖 (𝑡)$ 和 $𝑓_𝑗 (𝑡)$ 的内积为零。上述子载波序列之间的正交性很容易通过 $\Delta f \neq 0$ 推理证明。

频率间隔的影响：如果 $\Delta f$ 被选得足够小，使得 $2T \Delta f \cdot k$ 的范围包括整个频谱，这确保了所有的正弦波都在频率上不重叠。这样，每个正弦波在 $[0, T]$ 的完整周期内，它们的乘积积分为 0。

再下一步，将 $\cos(t)$ 也引入。容易证明，$\cos(t)$ 与 $\sin(t)$ 是正交的，也与整个 $\sin(kt)$ 的正交族相正交。同样，$\cos(kt)$ 也与整个 $\sin(kt)$ 的正交族相正交。

因此，序列模型扩展到 $\{ \sin(2T \Delta f \cdot t), \sin(2T \Delta f \cdot 2t), \ldots, \sin(2T \Delta f \cdot kt), \cos(2T \Delta f \cdot t), \cos(2T \Delta f \cdot 2t), \ldots, \cos(2T \Delta f \cdot kt) \}$ 也就顺理成章。

经过前两步的扩充，选好了2组正交序列 $\sin(kt)$ 和 $\cos(kt)$，这只是传输的"介质"。真正要传输的信息还需要调制在这些载波上，即 $\sin(t), \sin(2t), \ldots, \sin(kt)$ 分别幅度调制 $a1, a2, \ldots, ak$ 信号，$\cos(t), \cos(2t), \ldots, \cos(kt)$ 分别幅度调制 $b1, b2, \ldots, bk$ 信号。这 2n 组互相正交的信号同时发送出去，在空间上会叠加出怎样的波形呢？做简单的加法如下，得到信号 $f(t)$ 的表达式（公式 1-1）：
$$f(t) = \sum_{k=1}^{N} a_k \sin(2T \Delta f_k t) + \sum_{k=1}^{N} b_k \cos(2T \Delta f_k t)$$

其中，$a_k$ 和 $b_k$ 是系数， $\Delta f_k$ 是频率间隔， $T$ 是周期， $k = 1, 2, \ldots, N$。

现在，我们将 $f(t)$ 表示为复数形式。正弦波和余弦波可以用复指数形式表示，是因为它们是复指数函数的实部和虚部。具体来说，需要用到欧拉公式。

欧拉公式表明，对于任意实数 $\theta$，复指数 $e^{j\theta}$ 可以表示为：
$$e^{j\theta} = \cos(\theta) + j\sin(\theta)$$

这意味着 $e^{j\theta}$ 是一个单位圆上的点，它在复平面上的实部是 $\cos(\theta)$，虚部是 $\sin(\theta)$。

根据欧拉公式，我们可以写出实部和虚部为：
$$\sin(\theta) = \frac{e^{j\theta} - e^{-j\theta}}{2j}$$
$$\cos(\theta) = \frac{e^{j\theta} + e^{-j\theta}}{2}$$

这些公式是通过将 $e^{j\theta}$ 和 $e^{-j\theta}$ 的和与差进行组合得到的。

复指数函数 $e^{j\theta}$ 具有周期性、正交性和线性组合的性质，能够简洁地表示频率、幅度和相位。因此，使用复指数形式不仅有利于理论推导和分析，也方便了在数值计算和算法实现中的处理。

下面把正弦波序列和余弦波序列用复指数形式表示：
$$\sin(2T \Delta f_k t) = \frac{e^{j 2 \pi \Delta f_k t} - e^{-j 2 \pi \Delta f_k t}}{2j}$$

$$ \cos(2T \Delta f_k t) = \frac{e^{j 2 \pi \Delta f_k t} + e^{-j 2 \pi \Delta f_k t}}{2}$$

将 $f(t)$ 转换为复数形式：
$$f(t) = \sum_{k=1}^{N} a_k \cdot \frac{e^{j 2 \pi \Delta f_k t} - e^{-j 2 \pi \Delta f_k t}}{2j} + \sum_{k=1}^{N} b_k \cdot \frac{e^{j 2 \pi \Delta f_k t} + e^{-j 2 \pi \Delta f_k t}}{2}$$

这可以进一步简化为：
$$f(t) = \sum_{k=1}^{N} \left( \frac{a_k - jb_k}{2} e^{j 2 \pi \Delta f_k t} + \frac{a_k + jb_k}{2} e^{-j 2 \pi \Delta f_k t} \right)$$

我们定义复数形式的频谱成分 $F_k$ 为：$F_k = \frac{a_k - jb_k}{2}$

这样，信号 $f(t)$ 可以用 $F_k$ 表示为公式 1-2：
$$f(t) = \sum_{k=1}^{N} F_k e^{j 2 \pi \Delta f_k t} + \sum_{k=1}^{N} F_k^* e^{-j 2 \pi \Delta f_k t}$$

其中，$F_k^*$ 是 $F_k$ 的复共轭。这种复数形式更方便处理正弦波和余弦波的组合，特别在数字信号处理和通信系统中广泛应用。

上面的公式可以这样看：每个子载波序列都在发送自己的信号，互相交叠在空中，最终在接收端看到的信号就是 $f(t)$。接收端收到杂糅信号 $f(t)$ 后，再在每个子载波上分别作相乘后积分的操作，就可以取出每个子载波分别承载的信号了。

然后看看公式 1-1 和公式 1-2，其实这就是傅里叶级数。如果将离散化，那么就是离散傅立叶变换。所以才有了后面 OFDM 以 FFT 来实现的故事。

遵循古老的传统，$F$ 表示频域，$f$ 表示时域。由于频域信息就是各个频率分量的幅度和相位，信号 $f(t)$ 的频域信息直接由这些复数系数 $F_k$ 构成。每个 $F_k$ 对应频率 $\Delta f_k$ 的幅度和相位，这正是频域表示的核心内容。因此，可以从公式 1-2 中看出，每个子载波上调制的幅度就是频域信息。类似地，OFDM 传输的是频域信号。

## 频域 OFDM

在上一节中，我们推导了信号的复数形式，现在我们进一步讨论信号的频域和离散部分。我们已经看到，每个子载波在空中发送自己的信号，最终在接收端观测到的复合信号就是 $f(t)$。接收端通过在每个子载波上分别进行相乘和积分操作，可以提取出每个子载波承载的信号。

通过公式 1-1 和公式 1-2，我们可以看到，这实际上是傅里叶级数的应用。如果将其离散化，那么就是离散傅里叶变换（DFT）。这也是后面 OFDM 使用快速傅里叶变换（FFT）实现的基础。将在后面的章节中详细描述。

### 用 IFFT 实现 OFDM

在现代通信系统（如LTE）中，FFT（快速傅里叶变换）和 IFFT（逆快速傅里叶变换）在信号传输和接收中扮演着重要角色。让我们分解这个过程：

- **FFT**：在发送端，信号通常通过FFT变换，将时域信号转换为频域信号。FFT的作用是将连续的时域信号离散化，转换为离散的频域信号。
- **IDFT**：在接收端，收到的频域信号经过IDFT变换，重新转换回时域信号，使接收端能够还原原始的时域信号。

我们用公式（1-3）描述这一过程：

$$f(n) = \frac{1}{N} \sum_{k=1}^{N} F_k e^{\frac{2\pi i k n}{N}}$$

其中：

- $f(n)$ 是时域离散化后的序号为 $n$ 的点对应的值
- $F_k$ 是频域中频率序号为 $k$ 的点对应的值
- $N$ 是总的FFT/IFFT点数，通常等于信号的采样点数
- $n$ 和 $k$ 都是整数，范围是 1 到 N

这个公式描述了如何从频域 $F_k$ 转换回时域 $f(n)$，即从频域到时域的逆变换。

一种理解方式是联系第一章节的公式 1-2：公式 1-3 的右侧表达的物理意义与公式 1-2 相同，均表示不同子载波 $e^{j 2 \pi k n/N}$ 发送各自的信号 $F_k$，然后在时域上叠加形成 $f(n)$。不过现在叠加出来的时域是离散的时序抽样点。

另一种理解方式是：在一个 OFDM 符号的时长 $T$ 内，用 $N$ 个子载波各自发送一个信号 $F_k$（$k \in [1, N]$），等效于在时域上连续发送 $f_n$（$n \in [1, N]$） $N$ 个信号，每个信号发送 $T/N$ 的时长。

在 IFFT 实现 OFDM 中，发送端添加了 IFFT 模块，接收端添加了 FFT 模块：

- **发送端**：IFFT 模块的功能相当于计算出所有子载波在空中叠加后的波形。
- **接收端**：FFT 模块则是用数学方法快速去除各正交子载波，提取每个子载波携带的信号。

以传输 11011000 为例，首先分成 11, 01, 10, 00 四个符号，然后加入pilot 导频符号，对四个复数符号进行 IDFT/IFFT 得到时域发送信号，再加入 CPG 后转换成数字数据，升到中频高频发送。接收器则执行逆向操作。

<img src="docs/_static/Transmitter_Receiver_Architecture.png" width="550">

### 子载波分配

需要注意的是，在一个 OFDM 符号周期内，不一定每个子载波都携带数据。OFDM 系统中的子载波被划分为不同的子信道，其中一些子载波携带数据，其他则可能保持空闲或用于其他用途。具体分配方式包括：

- **数据传输子载波**：用于传输用户数据或其他信息。
- **保护子载波**：用于发送校验和纠错信息，增强系统稳定性和抗干扰能力。
- **空闲子载波**：保持空闲，用于提供保护间隔或其他用途。

### 采样规则

在 OFDM 系统中，采样间隔和子载波频率间隔之间需要满足一定规则，以确保信号的正交性和避免频谱重叠：

- **奈奎斯特采样定理**：采样率必须至少是信号带宽的两倍。每个子载波的带宽由子载波频率间隔决定，采样间隔需要至少是子载波频率间隔的两倍。
- **正交性要求**：子载波之间的频谱必须不重叠，保证子载波之间的正交性。
这些原则确保了 OFDM 系统的性能和可靠性。

## Reference

- https://blog.csdn.net/weixin_41483813/article/details/82622992
- https://blog.csdn.net/madongchunqiu/article/details/18614233
