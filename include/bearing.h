#ifndef BEARING_H
#define BEARING_H
#include <pthread.h>
#include "rtdb.h"

// NOTE - Bearing Thread declaration 
extern pthread_t bearing_th;
// var de controlo do estado da thread
extern volatile int bearing_run;

// NOTE - Funcao da thread de bearing
void *bearing_loop(void *arg);
// Da ao modulo bearing a rtdb
void bearing_set_rtdb(RTDB *db);

#endif
