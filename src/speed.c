#include <stdio.h>
#include <time.h>
#include "speed.h"
#include "desc_queue.h"
#include "dispatcher.h"
#include "time_utils.h"
#include "audio_io.h"
#include "config.h"
#include "lpf.h"

// NOTE - Thread de medição de Speed
pthread_t speed_th;
// Var de controlo do estado da thread
volatile int speed_run = 1;

static RTDB *g_db = NULL;
extern DescQueue *dispatcher_get_speed_queue(void);
extern AudioBuf bufA, bufB; // para release

void speed_set_rtdb(RTDB *db) { g_db = db; }

void *speed_loop(void *arg)
{
    (void)arg;
    const long PERIOD_MS = 200;
    struct timespec next_time;
    clock_gettime(CLOCK_MONOTONIC, &next_time);
    DescQueue *q = dispatcher_get_speed_queue();
    while (speed_run)
    {
        add_ms(&next_time, PERIOD_MS);
        AudioDesc d;
        if (desc_queue_pop(q, &d))
        {
            // NOTE - calculo do speed através da FFT

            // Converter o buffer em vetor de complexos
            static double complex X[ABUFSIZE_SAMPLES];
            for (int i = 0; i < d.len; ++i)
                X[i] = d.ptr[i] + 0.0 * I;

            // Calcular frequência dominante via FFT
            float freq_est = compute_dominant_freq(X, d.len, SAMP_FREQ);

            if (g_db)
                rtdb_set_speed(g_db, freq_est);
            printf("[SPEED] cycle: len=%d\n", d.len);
            audio_release_buffer(d.ptr);
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL);
    }
    return NULL;
}