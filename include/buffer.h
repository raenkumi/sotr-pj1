#ifndef BUFFER_H
#define BUFFER_H
#include <stdint.h>
#include "config.h"

// NOTE - Struct e inicialização do double buffer

// Estrutura dos novos buffers
typedef struct
{
	int16_t data[ABUFSIZE_SAMPLES]; // 4096 amostras por buffer
	volatile int full;				// volatile para sincronização segura do valor entre threads
	volatile int ready_to_consume;	// flag para indicar que o buffer está pronto para processamento
} AudioBuf;

void buffer_init(AudioBuf *b);
void buffer_release_nolock(int16_t *ptr, AudioBuf *a, AudioBuf *b);

#endif