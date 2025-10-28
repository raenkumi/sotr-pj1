#ifndef RTDB_H
#define RTDB_H
#include <pthread.h>

// NOTE - Real Time Data Base Struct
typedef struct {
    pthread_mutex_t mtx;
    
    float speed_hz;      
    int   bearing_fault; 
    int   direction;     
} RTDB;

void rtdb_init(RTDB *db);

// speed manipulation na rtdb
void rtdb_set_speed(RTDB *db, float hz);
float rtdb_get_speed(RTDB *db);

// bearing fault manipulation na rtdb
void rtdb_set_bearing_fault(RTDB *db, int fault);
int  rtdb_get_bearing_fault(RTDB *db);


#endif