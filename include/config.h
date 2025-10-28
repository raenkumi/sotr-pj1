#ifndef CONFIG_H
#define CONFIG_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MONO 1
#define SAMP_FREQ 44100
#define FORMAT AUDIO_U16
#define ABUFSIZE_SAMPLES 4096
// Frequencia de corte do lpf
#define CUTOFF_HZ 1000
// limite maximo de frequencia util para o calculo do speed
#define MAX_USEFUL_FREQ 10000


#define DESCRIPTOR_QUEUE_CAPACITY 8

#endif