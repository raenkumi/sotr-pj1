#include "rtdb.h"

void rtdb_init(RTDB *db)
{
    pthread_mutex_init(&db->mtx, NULL);
    db->speed_hz = 0.0f;
    db->bearing_fault = 0;
    db->direction = 0;
}

void rtdb_set_speed(RTDB *db, float hz)
{
    pthread_mutex_lock(&db->mtx);
    db->speed_hz = hz;
    pthread_mutex_unlock(&db->mtx);
}

float rtdb_get_speed(RTDB *db)
{
    float v;
    pthread_mutex_lock(&db->mtx);
    v = db->speed_hz;
    pthread_mutex_unlock(&db->mtx);
    return v;
}

void rtdb_set_bearing_fault(RTDB *db, int fault)
{
    pthread_mutex_lock(&db->mtx);
    db->bearing_fault = fault ? 1 : 0;
    pthread_mutex_unlock(&db->mtx);
}

int rtdb_get_bearing_fault(RTDB *db)
{
    int v;
    pthread_mutex_lock(&db->mtx);
    v = db->bearing_fault;
    pthread_mutex_unlock(&db->mtx);
    return v;
}
