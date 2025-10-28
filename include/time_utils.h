#ifndef TIME_UTILS_H
#define TIME_UTILS_H
#include <time.h>

// NOTE - Função auxiliar para adicionar ms a um timespec 
static inline void add_ms(struct timespec *t, long ms)
{
    t->tv_nsec += (ms % 1000) * 1000000L;
    t->tv_sec += ms / 1000 + t->tv_nsec / 1000000000L;
    t->tv_nsec %= 1000000000L;
}

#endif