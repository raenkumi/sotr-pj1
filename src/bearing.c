#include <time.h>
#include <stdio.h>
#include "bearing.h"
#include "dispatcher.h"
#include "desc_queue.h"
#include "buffer.h"
#include "config.h"
#include "lpf.h"
#include "time_utils.h"
#include "audio_io.h"

// NOTE - Thread de medição de Bearing
pthread_t bearing_th;
// Var de controlo do estado da thread
volatile int bearing_run = 1;

static RTDB *g_db = NULL;
extern DescQueue *dispatcher_get_bearing_queue(void);
extern AudioBuf bufA, bufB;

void bearing_set_rtdb(RTDB *db) { g_db = db; }

void *bearing_loop(void *arg)
{
    (void)arg;
    const long PERIOD_MS = 1000;
    struct timespec next_time;
    clock_gettime(CLOCK_MONOTONIC, &next_time);

    // Reutilizamos a mesma queue do speed para consumir blocos
    DescQueue *q = dispatcher_get_bearing_queue();

    // thresholds (ajusta conforme necessário)
    const float MOTOR_MIN = 200.0f;
    const float MOTOR_MAX = 5000.0f;
    const float LOWF_TH = 150.0f;
    const float REL_TH = 0.25f;

    while (bearing_run)
    {
        add_ms(&next_time, PERIOD_MS);

        AudioDesc d;
        if (desc_queue_pop(q, &d))
        {
            int fault = compute_bearing_issue_freq(d.ptr, d.len, SAMP_FREQ,
                                                   MOTOR_MIN, MOTOR_MAX,
                                                   LOWF_TH, REL_TH);
            if (g_db)
                rtdb_set_bearing_fault(g_db, fault);
            
            printf("[BEARING] cycle: len=%d fault=%d\n", d.len, fault);
            
            audio_release_buffer(d.ptr);
        }

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);
    }
    return NULL;
}
