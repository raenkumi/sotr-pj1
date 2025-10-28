#ifndef LPF_H
#define LPF_H
#include <stdint.h>
#include <complex.h>

// NOTE - Funcao auxiliar para evitar saturacao no lpf
float clampf(float v, float min, float max);
// NOTE - Funcao do filtro passa baixo
void filterLP(uint32_t cof, uint32_t sampleFreq, uint8_t *buffer, uint32_t nSamples);

// NOTE - FFT cálculo da frequência dominante
float compute_dominant_freq(double complex *X, int N, int fs);

// NOTE - FFT para calculo de bearing issue
// Deteção de anomalias em rolamentos por energia LF relativa
// Retorna 1 se fault-like 0 caso contrario
int compute_bearing_issue_freq(const int16_t *x, int N, int fs,
                               float motor_min_hz, float motor_max_hz,
                               float low_freq_thresh_hz,
                               float rel_amp_thresh);

#endif