#ifndef DESC_QUEUE_H
#define DESC_QUEUE_H
#include <pthread.h>
#include <stdint.h>
#include "config.h"

// NOTE - Descritor para as filas do dispatcher
typedef struct
{
	int16_t *ptr; // ponteiro para os dados do buffer cheio
	int len;	  // numero de amostras
} AudioDesc;

// NOTE - Estrutura para as filas do dispatcher
// O dispatcher contem uma fila para cada thread dedicada compostas pelos descritores
typedef struct
{
	AudioDesc desc[DESCRIPTOR_QUEUE_CAPACITY];
	int head;
	int tail;
	int count;
	pthread_mutex_t mtx;
} DescQueue;

void desc_queue_init(DescQueue *q);
int desc_queue_push(DescQueue *q, AudioDesc d);
int desc_queue_pop(DescQueue *q, AudioDesc *out);

#endif