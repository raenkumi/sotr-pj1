#include "buffer.h"

// NOTE - Inicializa todos os campos do buffer a zero
void buffer_init(AudioBuf *b)
{
    b->full = 0;
    b->ready_to_consume = 0;
    for (int i = 0; i < ABUFSIZE_SAMPLES; i++)
        b->data[i] = 0;
}

// NOTE - Funcao para as threads consumidoras libertarem o buffer 
void buffer_release_nolock(int16_t *ptr, AudioBuf *a, AudioBuf *b)
{

    if (ptr == a->data)
    {
        a->full = 0;
        a->ready_to_consume = 0;
    }
    else if (ptr == b->data)
    {
        b->full = 0;
        b->ready_to_consume = 0;
    }

}