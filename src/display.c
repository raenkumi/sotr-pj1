#include <stdio.h>
#include <time.h>
#include "display.h"
#include "time_utils.h"
#include "rtdb.h"

volatile int display_run = 1;
pthread_t display_th;
static RTDB *g_db = NULL;

void display_set_rtdb(RTDB *db) { g_db = db; }

void *display_loop(void *arg)
{
    (void)arg;
    const long PERIOD_MS = 300;
    struct timespec next_time;
    clock_gettime(CLOCK_MONOTONIC, &next_time);
    while (display_run)
    {
        add_ms(&next_time, PERIOD_MS);
        if (g_db)
        {
            float hz = rtdb_get_speed(g_db);
            float rpm = hz * 60.0f;
            int fault = rtdb_get_bearing_fault(g_db);

            printf("[DISPLAY] speed: %.1f Hz (%.0f rpm) | bearing: %s\n",
                   hz, rpm, fault ? "FAULT" : "OK");
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);
    }
    return NULL;
}