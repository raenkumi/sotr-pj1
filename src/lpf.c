#include <math.h>
#include "lpf.h"
#include "fft.h"
#include "config.h"
#include <stdio.h>
#include <complex.h>

/* **************************************************************
 * Audio processing example:
 *  	Applies a low-pass filter
 * 		Args are cutoff frequency, buffer and nsamples in buffer
 *
 * 		Simple realization derived from the discretization on an analog RC low-pass filter. See e.g.
 * 			https://en.wikipedia.org/wiki/Low-pass_filter#Simple_infinite_impulse_response_filter
 * ************************************************************ */
// Funcao para evitar saturacao usada no LP filter
float clampf(float v, float min, float max)
{
	if (v < min)
		return min;
	if (v > max)
		return max;
	return v;
}

// Funcao do filtro passa baixo alterada
// Agora recebe valores com sinal
void filterLP(uint32_t cof, uint32_t sampleFreq, uint8_t *buffer, uint32_t nSamples)
{

	int i;

	int16_t *origBuffer; /* Agora em S16 com sinal */

	float alfa, beta;
	float y, s;

	/* Compute alfa and beta multipliers */
	alfa = (2.0f * (float)M_PI / (float)sampleFreq * (float)cof) / ((2.0f * (float)M_PI / (float)sampleFreq * (float)cof) + 1.0f);
	beta = 1.0f - alfa;

	/* Get pointer to buffer of the right type */
	origBuffer = (int16_t *)buffer;

	y = (float)origBuffer[0];

	for (i = 0; i < (int)nSamples; i++)
	{
		y = alfa * (float)origBuffer[i] + beta * y;
		s = clampf(y, -32768.0f, 32767.0f);
		origBuffer[i] = (int16_t)s;
	}

	return;
}

// NOTE - FFT cálculo da frequência dominante para o speed
float compute_dominant_freq(double complex *X, int N, int fs)
{
    fftCompute(X, N);

    float freqs[N / 2];
    float amps[N / 2];
    fftGetAmplitude(X, N, fs, freqs, amps);

	// REVIEW - comentar 
	// O lpf reduz a aplitude das frequencias altas
	// Mas a freq dominante continua a ser 3 kHz
	// === DEBUG: verificar amplitude em torno dos 3 kHz ===
	
	//int k = (int)roundf(3000.0f * N / (float)fs);  // índice que corresponde a 3 kHz
	//printf("[DEBUG] mag@3kHz = %.3f\n", amps[k]);


    float f_peak = 0.0f;
    float A_peak = 0.0f;

    for (int i = 0; i < N / 2; i++)
    {
        if (amps[i] > A_peak && freqs[i] < MAX_USEFUL_FREQ)
        {
            A_peak = amps[i];
            f_peak = freqs[i];
        }
    }

    return f_peak;
}

int compute_bearing_issue_freq(const int16_t *x, int N, int fs,
                             float motor_min_hz, float motor_max_hz,
                             float low_freq_thresh_hz,
                             float rel_amp_thresh)
{
    // Copia samples para vetor complexo 
    static double complex Xbuf[ABUFSIZE_SAMPLES];
    for (int i = 0; i < N; ++i) Xbuf[i] = (double)x[i];

    // janela simples para reduzir leakage
    for (int i = 0; i < N; ++i) {
        double w = 0.5 * (1.0 - cos(2.0*M_PI*i/(N-1))); // Hann
        Xbuf[i] *= w;
    }

    // FFT e espectro de amplitudes
    fftCompute(Xbuf, N);

    float fk[N/2];
    float Ak[N/2];
    fftGetAmplitude(Xbuf, N, fs, fk, Ak);

    // Amplitude máxima na banda do motor
    float Amax_motor = 0.0f;
    for (int k = 0; k < N/2; ++k) {
        if (fk[k] >= motor_min_hz && fk[k] <= motor_max_hz) {
            if (Ak[k] > Amax_motor) Amax_motor = Ak[k];
        }
    }
    if (Amax_motor <= 0.0f) return 0; // sem referência assumimos normal

    // Procura picos anómalos em low freqs
    for (int k = 0; k < N/2; ++k) {
        if (fk[k] < low_freq_thresh_hz) {
            if (Ak[k] > rel_amp_thresh * Amax_motor) {
                // fault-like
				return 1;
            }
        }
    }
    return 0;
}

