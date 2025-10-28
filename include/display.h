#ifndef DISPLAY_H
#define DISPLAY_H
#include <pthread.h>
#include "rtdb.h"

// NOTE - Thread display
extern pthread_t display_th;
// Var de controlo do estado da thread
extern volatile int display_run;

// NOTE - Funcao da thread de display
void *display_loop(void *arg);
// Da ao modulo display a rtdb
void display_set_rtdb(RTDB *db);

#endif