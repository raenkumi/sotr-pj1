#ifndef AUDIO_IO_H
#define AUDIO_IO_H
#include <SDL.h>
#include <stdint.h>
#include "buffer.h"

extern SDL_AudioDeviceID gRecDev;

extern AudioBuf bufA, bufB;
extern AudioBuf *curBuf;

void audio_recording_callback(void *userdata, Uint8 *stream, int len);

// NOTE - wrapper que faz o lock/unlock e liberta o buffer
void audio_release_buffer(int16_t *ptr);

#endif