#ifndef DISPATCHER_H
#define DISPATCHER_H
#include <pthread.h>
#include "desc_queue.h"

// Variável de controlo do estado da thread
// extern para partilhar a mesma variavel global entre os modulos
extern volatile int dispatcher_run;
extern pthread_t dispatcher_th;

void *dispatcher_loop(void *arg);

// NOTE - Getters para as filas do dispatcher
// Servem para que as threads consumidoras acedam às respetivas filas do dispatcher
DescQueue *dispatcher_get_speed_queue(void);
DescQueue *dispatcher_get_bearing_queue(void);
DescQueue *dispatcher_get_direction_queue(void);

// contador de blocos para critério de paragem
int dispatcher_blocks_count(void);

#endif