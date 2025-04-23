/* SPI Slave example, receiver (uses SPI Slave driver to communicate with sender)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "app_ifft.h"
#include "IQmathLib.h"
#define PI 3.14159265358979323846f
#define PI_IQ  _IQ(3.14159265358979323846)
#define PI_IQ_N2  _IQ(-6.28318530717958647692)
const int bitReverseTable[64] = {
    0, 32, 16, 48, 8, 40, 24, 56,
    4, 36, 20, 52, 12, 44, 28, 60,
    2, 34, 18, 50, 10, 42, 26, 58,
    6, 38, 22, 54, 14, 46, 30, 62,
    1, 33, 17, 49, 9, 41, 25, 57,
    5, 37, 21, 53, 13, 45, 29, 61,
    3, 35, 19, 51, 11, 43, 27, 59,
    7, 39, 23, 55, 15, 47, 31, 63
};

void IRAM_ATTR fft_iq(Complex_Iq *X, int inverse) 
{
    int log2N = 6;
    int N = 64;
    Complex_Iq *temp = (Complex_Iq *)malloc(64 * sizeof(Complex_Iq));

    // Bit-reversed addressing permutation
    for (int i = 0; i < N; i++) {
        temp[i] = X[bitReverseTable[i]];
    }
    for (int i = 0; i < N; i++) {
        X[i] = temp[i];
    }

    // Cooley-Tukey iterative FFT
    for (int s = 1; s <= log2N; ++s) {
        int m = 1 << s; // 2 power s
        int m2 = m >> 1; // m/2
        Complex_Iq w;
        w.real = _IQ16(1.0);
        w.imag = _IQ16(0.0);
        Complex_Iq wm;
        _iq16  angle = _IQ16div(PI_IQ_N2, m);
        wm.real = _IQ16cos(angle);                                                                                    
        wm.imag = _IQ16sin(angle);
        if (inverse) wm.imag = -wm.imag;

        for (int j = 0; j < m2; ++j) {
            for (int k = j; k < N; k += m) {
                Complex_Iq t, u;
                u = X[k];
                t.real = _IQ16mpy(w.real , X[k + m2].real) - _IQ16mpy(w.imag , X[k + m2].imag);
                t.imag = _IQ16mpy(w.real , X[k + m2].imag) + _IQ16mpy(w.imag , X[k + m2].real);
                X[k].real = u.real + t.real;
                X[k].imag = u.imag + t.imag;
                X[k + m2].real = u.real - t.real;
                X[k + m2].imag = u.imag - t.imag;
            }
            float tmpReal = _IQ16mpy(w.real , wm.real) - _IQ16mpy(w.imag , wm.imag);
            w.imag = _IQ16mpy(w.real , wm.imag) + _IQ16mpy(w.imag , wm.real);
            w.real = tmpReal;
        }
    }

    // Scale for inverse FFT
    if (inverse) {
        for (int i = 0; i < N; i++) {
            X[i].real = _IQdiv64(X[i].real);
            X[i].imag = _IQdiv64(X[i].imag);
        }
    }

    free(temp);
}

// Bit reversal of given index 'x' with 'log2n' bits
unsigned int inline bitReverse(unsigned int x, int log2n) {
    int n = 0;
    for (int i = 0; i < log2n; i++) {
        n <<= 1;
        n |= (x & 1);
        x >>= 1;
    }
    return n;
}
void IRAM_ATTR fft(Complex *X, int N, int inverse) 
{
    int log2N = log2(N);
    Complex *temp = (Complex *)malloc(N * sizeof(Complex));

    // Bit-reversed addressing permutation
    for (int i = 0; i < N; i++) {
        temp[i] = X[bitReverse(i, log2N)];
    }
    for (int i = 0; i < N; i++) {
        X[i] = temp[i];
    }

    // Cooley-Tukey iterative FFT
    for (int s = 1; s <= log2N; ++s) {
        int m = 1 << s; // 2 power s
        int m2 = m >> 1; // m/2
        Complex w;
        w.real = 1.0;
        w.imag = 0.0;
        Complex wm;
        wm.real = cosf(-2.0f * PI / m);
        wm.imag = sinf(-2.0f * PI / m);
        if (inverse) wm.imag = -wm.imag;

        for (int j = 0; j < m2; ++j) {
            for (int k = j; k < N; k += m) {
                Complex t, u;
                u = X[k];
                t.real = w.real * X[k + m2].real - w.imag * X[k + m2].imag;
                t.imag = w.real * X[k + m2].imag + w.imag * X[k + m2].real;
                X[k].real = u.real + t.real;
                X[k].imag = u.imag + t.imag;
                X[k + m2].real = u.real - t.real;
                X[k + m2].imag = u.imag - t.imag;
            }
            float tmpReal = w.real * wm.real - w.imag * wm.imag;
            w.imag = w.real * wm.imag + w.imag * wm.real;
            w.real = tmpReal;
        }
    }

    // Scale for inverse FFT
    if (inverse) {
        for (int i = 0; i < N; i++) {
            X[i].real /= N;
            X[i].imag /= N;
        }
    }

    free(temp);
}
float complex_magnitude_iq(Complex_Iq z) {
    return _IQ16toF(_IQ16mag(z.real, z.imag));
} 

float complex_phase_iq(Complex_Iq z) {
    return _IQ16toF(_IQ16atan2(z.imag, z.real));
}

float complex_magnitude(Complex z) {
    return sqrt(z.real * z.real + z.imag * z.imag);
}
float complex_phase(Complex z) {
    return atan2(z.imag, z.real);
}