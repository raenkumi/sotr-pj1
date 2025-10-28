#ifndef SPEED_H
#define SPEED_H
#include <pthread.h>
#include "desc_queue.h"
#include "rtdb.h"

// NOTE - Thread de medição de Speed
extern pthread_t speed_th;
// Var de controlo do estado da thread
extern volatile int speed_run;

// NOTE - Funcao da thread de speed
void *speed_loop(void *arg);
// Da ao modulo speed a rtdb
void speed_set_rtdb(RTDB *db);

#endif