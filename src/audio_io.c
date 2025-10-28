#include <string.h>
#include "audio_io.h"

SDL_AudioDeviceID gRecDev = 0;

AudioBuf bufA, bufB;
AudioBuf *curBuf = &bufA;

void audio_recording_callback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata;
    
    // NOTE - Recolha de amostras para o buffer antigo
	// Foi retirada pois agora recolhemos as amostras para o double buffer

	/* Copy bytes acquired from audio stream */
	// memcpy(&gRecordingBuffer[ gBufferBytePosition ], stream, len);

	/* Update buffer pointer */
	// gBufferBytePosition += len;

	// NOTE - Recolher os dados para os buffers A e B e ir alternando
	// Através do curBuf

	// Determinação do número exato de bytes a copiar
	int expected_bytes = sizeof(curBuf->data);
	int tocopy = (len < expected_bytes) ? len : expected_bytes;

	// Copia dos dados para o current Buffer
	if (!curBuf->full && !curBuf->ready_to_consume)
	{
		// cast para o memcpy interpretar o inicio do array data como ponteiro para bytes
		// Pois ele espera um ponteiro para bytes
		memcpy((Uint8 *)curBuf->data, stream, tocopy);
		// REVIEW - estamos a considerar full quando len < expected_bytes
		// Não é correto, devemos alterar mais à frente
		curBuf->full = 1;
		// Efetuar a troca de buffer
		curBuf = (curBuf == &bufA) ? &bufB : &bufA;
	}

	/*NOTE - Ate agora no que implementamos, ainda não temos codigo
	para esvaziar os buffers. Portanto teoricamente ele vai aceder
	ao 1º buffer, echer, depois ao 2º, encher, e depois vai dropar todos
	os blocos a partir de ai pois nao temos maneira de os esvaziar implementada ainda*/

}

void audio_release_buffer(int16_t *ptr) {
    SDL_LockAudioDevice(gRecDev);
    buffer_release_nolock(ptr, &bufA, &bufB);
    SDL_UnlockAudioDevice(gRecDev);
}