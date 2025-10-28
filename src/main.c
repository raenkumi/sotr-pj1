#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <SDL.h>
#include <pthread.h>
#include <sched.h>
#include "config.h"
#include "rtdb.h"
#include "buffer.h"
#include "audio_io.h"
#include "dispatcher.h"
#include "speed.h"
#include "display.h"
#include "bearing.h"

// globais definidas em audio_io.c
extern AudioBuf bufA, bufB;
extern AudioBuf *curBuf;

static void set_thread_prio(pthread_t th, int prio)
{
    struct sched_param sp;
    sp.sched_priority = prio;
    int ret = pthread_setschedparam(th, SCHED_FIFO, &sp);
    if (ret != 0)
        perror("pthread_setschedparam");
}

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    RTDB db;
    rtdb_init(&db);
    buffer_init(&bufA);
    buffer_init(&bufB);
    curBuf = &bufA;

    // Listar dispositivos de captura e pedir índice (compatível com original)
    int ndev = SDL_GetNumAudioDevices(SDL_TRUE);
    if (ndev < 1)
    {
        printf("No capture devices: %s\n", SDL_GetError());
        return 1;
    }
    for (int i = 0; i < ndev; ++i)
    {
        printf("%d - %s\n", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
    }
    int index = 0;
    if (argc >= 2)
        index = atoi(argv[1]);
    else
    {
        printf("Choose audio index: ");
        fflush(stdout);
        if (scanf("%d", &index) != 1)
        {
            puts("Invalid input");
            return 1;
        }
    }
    if (index < 0 || index >= ndev)
    {
        printf("Invalid device ID. Must be 0..%d\n", ndev - 1);
        return 1;
    }
    printf("Using audio capture device %d - %s\n", index, SDL_GetAudioDeviceName(index, SDL_TRUE));

    // Abrir device de gravação
    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = SAMP_FREQ;
    desired.format = FORMAT;
    desired.channels = MONO;
    desired.samples = ABUFSIZE_SAMPLES;
    desired.callback = audio_recording_callback;

    SDL_AudioSpec obtained;
    SDL_AudioDeviceID rec = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(index, SDL_TRUE), SDL_TRUE,
                                                &desired, &obtained, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if (!rec)
    {
        printf("Open capture failed: %s\n", SDL_GetError());
        return 1;
    }
    gRecDev = rec;

    // threads
    if (pthread_create(&dispatcher_th, NULL, dispatcher_loop, NULL) != 0)
    {
        perror("dispatcher");
        return 1;
    }

    speed_set_rtdb(&db);
    if (pthread_create(&speed_th, NULL, speed_loop, NULL) != 0)
    {
        perror("speed");
        return 1;
    }

    bearing_set_rtdb(&db);
    if (pthread_create(&bearing_th, NULL, bearing_loop, NULL) != 0)
    {
        perror("bearing");
        return 1;
    }

    display_set_rtdb(&db);
    if (pthread_create(&display_th, NULL, display_loop, NULL) != 0)
    {
        perror("display");
        return 1;
    }

    // Prioridades RT - entre 1 e 99
    set_thread_prio(speed_th, 60);
    set_thread_prio(bearing_th, 50);
    set_thread_prio(display_th, 40);

    // Iniciar gravacao
    SDL_PauseAudioDevice(rec, SDL_FALSE);

    // Critério de paragem - parar quando blocksdispatched >= 50
    while (1)
    {
        SDL_LockAudioDevice(rec);
        if (dispatcher_blocks_count() >= 50)
        {
            SDL_PauseAudioDevice(rec, SDL_TRUE);
            SDL_UnlockAudioDevice(rec);
            break;
        }
        SDL_UnlockAudioDevice(rec);
        SDL_Delay(5);
    }

    // parar threads e fechar
    dispatcher_run = 0;
    speed_run = 0;
    bearing_run = 0;
    display_run = 0;

    pthread_join(speed_th, NULL);
    pthread_join(bearing_th, NULL);
    pthread_join(display_th, NULL);
    pthread_join(dispatcher_th, NULL);

    SDL_CloseAudioDevice(rec);
    SDL_Quit();
    return 0;
}
