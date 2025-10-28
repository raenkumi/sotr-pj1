#include <stdio.h>
#include <SDL.h>
#include "dispatcher.h"
#include "desc_queue.h"
#include "audio_io.h"
#include "lpf.h"

// NOTE - Thread
// Criar variavel para guardar o identificador da thread
pthread_t dispatcher_th;
// Variável de controlo do estado da thread
volatile int dispatcher_run = 1;

// Filas de descritores tem de ser static para poderem ser acedidas pelas threads
static DescQueue q_speed;
static DescQueue q_bearing;
static DescQueue q_direction;

// exposição controlada das filas do dispatcher 
// para as threads consumidoras poderem aceder às respetivas filas
DescQueue *dispatcher_get_speed_queue(void) {
    return &q_speed;
}

DescQueue *dispatcher_get_bearing_queue(void)
{
    return &q_bearing;
}

DescQueue *dispatcher_get_direction_queue(void) {
    return &q_direction;
}

// variavel que determina o criterio de paragem de gravação
static int blocksdispatched = 0;

// Funao para outros modulos obterem o n de blocos despachados
int dispatcher_blocks_count(void) {
    return blocksdispatched;
}

// NOTE - Thread Dispatcher function
// Lock e Unlock para evitar race condicions com a callback
void *dispatcher_loop(void *arg)
{
    // Evita o warning “unused parameter” porque a função
    // de thread (pthread_create) tem de receber void *arg
    (void)arg;

    // se a thread é chamada, inicializamos as filas
    desc_queue_init(&q_speed);
    desc_queue_init(&q_bearing);
    desc_queue_init(&q_direction);

    while (dispatcher_run)
    {
        
        // Temos de bloquear porque vamos mexer nos buffers
        SDL_LockAudioDevice(gRecDev);
        
        if (bufA.full && !bufA.ready_to_consume)
        {
            
            bufA.ready_to_consume = 1;
            AudioDesc d = {.ptr = bufA.data, .len = ABUFSIZE_SAMPLES};
            SDL_UnlockAudioDevice(gRecDev);

            // NOTE - Filtrar o bloco antes de fazer push
            filterLP(CUTOFF_HZ, SAMP_FREQ, (uint8_t*)d.ptr, d.len);

            desc_queue_push(&q_speed, d);
            desc_queue_push(&q_bearing, d);
            desc_queue_push(&q_direction, d);
            blocksdispatched++;
            //printf("[DISPATCH] push A (total=%d)\n", blocksdispatched);
        }
        else if (bufB.full && !bufB.ready_to_consume)
        {
            
            bufB.ready_to_consume = 1;
            AudioDesc d = {.ptr = bufB.data, .len = ABUFSIZE_SAMPLES};
            SDL_UnlockAudioDevice(gRecDev);

            // NOTE - Filtrar o bloco antes de fazer push
            filterLP(CUTOFF_HZ, SAMP_FREQ, (uint8_t*)d.ptr, d.len);

            desc_queue_push(&q_speed, d);
            desc_queue_push(&q_bearing, d);
            desc_queue_push(&q_direction, d);
            blocksdispatched++;
            printf("[DISPATCH] push B (total=%d)\n", blocksdispatched);
        }
        else
        {
            SDL_UnlockAudioDevice(gRecDev);
            SDL_Delay(2);
        }
    }
    return NULL;
}