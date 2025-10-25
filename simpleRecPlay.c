/* ***************************************************************************
 * Paulo Pedreiras Sept 2024
 * pbrp@ua.pt
 * 
 * Example code for acquiring and processing sound using SDL
 * - There are several possibilities, set by #define directives
 * 		- Record sound and play it back
 * 		- Generate a sine wave (configurable frequency and duration)
 * 		- Add an echo (works fine for syntetich signals)
 * 		- Computing the FFT of a signal
 * 		- Debug functions (print samples, max/min, ...) * 
 * 
 * 	- All functiosn are very crude, with almost no validations. Use with care.
 * 		Bugs are almost guranteed to exist ... please report them to pbrp@ua.pt
 * 
 * Record/playback sound code adapted from:
 * 	https://gist.github.com/cpicanco/12147c60f0b62611edb1377922f443cd
 * 
 * Main SDL 2 doc.
* 	https://wiki.libsdl.org/SDL2/FrontPage
 * *************************************************************************/
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <complex.h>
#include "fft/fft.h"
#include "pthread.h"

// NOTE - Thread
// Criar variavel para guardar o identificador da thread
static pthread_t dispatcher_th;
// Variável de controlo do estado da thread
static volatile int dispatcher_run = 1;
// variavel que determina o criterio de paragem de gravação
static volatile int blocksdispatched = 0;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MONO 1 					/* Sample and play in mono (1 channel) */
#define SAMP_FREQ 44100			/* Sampling frequency used by audio device */
#define FORMAT AUDIO_U16		/* Format of each sample (signed, unsigned, 8,16 bits, int/float, ...) */
#define ABUFSIZE_SAMPLES 4096	/* Audio buffer size in sample FRAMES (total samples divided by channel count) */


// NOTE - Struct e inicialização do double buffer

// Estrutura dos novos buffers
typedef struct {	
	int16_t data[ABUFSIZE_SAMPLES]; // 4096 amostras por buffer
	volatile int full; 				// volatile para sincronização segura do valor entre threads
} AudioBuf;

// static - variavel global visivel apenas dentro deste .c
// Inicialização dos buffers

// .full = 0 inicializa a var full do buffer respetivo a zero
// data é inicializado todo a zero automaticamente porque é o padrao das variaveis estaticas em C
// full tambem seria inicializado a zero mas decidi colocar porque é uma flag de controlo e por isso achei que deveria ficar explicito
static AudioBuf bufA = { .full = 0 }; 
static AudioBuf bufB = { .full = 0 };
// Ponteiro para o buffer que esta a receber amostras
// Vai ser a partir do curBuf que vamos aceder aos dados curBuf->data[i]
// Nao a partir dos buffers diretamente
// Este ponteiro irá mudar o buffer apontado quando o current buffer estiver cheio
// Isto é, quando encher o buffer atual de amostras novas
// A ideia é que o tempo de recolha de amostras seja sempre inferior
// ao tempo de processamento das amostras no outro buffer para que não se perca informação
static AudioBuf *curBuf = &bufA;

// Variável para tornar o audio device acessivel a outras threads
// Nao apenas ao main. Depois no main fazemos gRecDev = recordingDeviceId
static SDL_AudioDeviceID gRecDev = 0;


const int MAX_RECORDING_DEVICES = 10;		/* Maximum allowed number of souns devices that will be detected */

//Maximum recording time
const int MAX_RECORDING_SECONDS = 5;

//Maximum recording time plus padding
const int RECORDING_BUFFER_SECONDS = MAX_RECORDING_SECONDS + 1;

//Receieved audio spec
SDL_AudioSpec gReceivedRecordingSpec;
SDL_AudioSpec gReceivedPlaybackSpec;

//Recording data buffer
Uint8 * gRecordingBuffer = NULL;

//Size of data buffer
Uint32 gBufferByteSize = 0;

//Position in data buffer
Uint32 gBufferBytePosition = 0;

//Maximum position in data buffer for recording
Uint32 gBufferByteMaxPosition = 0;

int gRecordingDeviceCount = 0;

char y;

/* ************************************************************** 
 * Callback issued by the capture device driver. 
 * Args are:
 * 		userdata: optional user data passed on AudioSpec struct
 * 		stream: buffer of samples acquired 
 * 		len: length of buffer ***in bytes*** (not # of samples) 
 * 
 * Just copy the data to the application buffer and update index 
 * **************************************************************/
void audioRecordingCallback(void* userdata, Uint8* stream, int len )
{
	// NOTE - Recolha de amostras para o buffer antigo 
	// Foi retirada pois agora recolhemos as amostras para o double buffer 

	/* Copy bytes acquired from audio stream */
	//memcpy(&gRecordingBuffer[ gBufferBytePosition ], stream, len);

	/* Update buffer pointer */
	//gBufferBytePosition += len;


	// NOTE - Recolher os dados para os buffers A e B e ir alternando
	// Através do curBuf

	// Determinação do número exato de bytes a copiar
	int expected_bytes = sizeof(curBuf->data);
	int tocopy = (len < expected_bytes) ? len : expected_bytes;

	// Copia dos dados para o current Buffer
	if (!curBuf->full) {
		// cast para o memcpy interpretar o inicio do array data como ponteiro para bytes
		// Pois ele espera um ponteiro para bytes
		memcpy((Uint8*)curBuf->data, stream, tocopy);
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

/* *********************************************************************************** 
 * Callback issued by the playback device driver. 
 * Args are:
 * 		userdata: optional user data passed on AudioSpec struct
 * 		stream: buffer o samples acquired 
 * 		len: length of buffer ***in bytes*** (not # of samples) 
 * 
 * Reverse of above. Copy buffer data to the devide-river buffer and update index 
 * **********************************************************************************/
void audioPlaybackCallback(void* userdata, Uint8* stream, int len )
{
	/* Copy buffer with audio samples to stream for playback */
	memcpy(stream, &gRecordingBuffer[ gBufferBytePosition ], len);

	/* Update buffer index */
	gBufferBytePosition += len;
}


/* ***********************************************
 * Debug function: 
 *       Prints the buffer contents - 8 bit samples * 
 * **********************************************/
void printSamplesU8(uint8_t * buffer, int size) {
	int i=0;
	
	printf("\n\r Samples: \n\r");
	for(i = 0; i < size; i++) {
		printf("%3u ",buffer[i]);
		if((i%20) == 0)
			printf("\n\r");
	}		
}

/* ***********************************************
 * Debug function: 
 *       Prints the buffer contents - uint16 samples * 
 * **********************************************/
void printSamplesU16(uint8_t * buffer, int nsamples) {
	int i=0;
	uint16_t * bufu16 = (uint16_t *)buffer;
	 
	printf("\n\r Samples: \n\r");
	for(i = 0; i < nsamples; i++) {
		printf("%5u ",bufu16[i]);
		if((i%20) == 0)
			printf("\n\r");
	}		
}

/* **************************************************
 * Audio processing example:
 *     Adds two echoes - uint16 sample format
 * 		Be carefull with saturation 
 * ************************************************* */ 
void addEchoU16(uint32_t delay1MS, uint32_t delay2MS, float gain1, float gain2, uint8_t * buffer, uint32_t nSamples)
{	
		
	int delaySamples1, delaySamples2;
	int i=0;
		
	uint16_t * procBuffer; 	/* Temporary buffer */
	uint16_t * origBuffer; 	/* Pointer to original buffer, with right sample type (UINT16 in the case) */
		
	/* Get pointer to buffer of the right type */
	origBuffer = (uint16_t *)buffer;
	
	/* Convert time to number of samples */	
	delaySamples1 = SAMP_FREQ * (float)delay1MS/1000;
	delaySamples2 = SAMP_FREQ * (float)delay2MS/1000;
	
	/* allocate temporary buffer and copy data to it*/
	procBuffer = (uint16_t *)malloc(nSamples*sizeof(uint16_t));	
	memcpy((uint8_t *)procBuffer, (uint8_t *)origBuffer, nSamples*sizeof(uint16_t));
	
	/* Add the echoes */		
	for(i = delaySamples1; i < nSamples; i++) {				
		procBuffer[i] += (uint16_t)(origBuffer[i-delaySamples1] * gain1);		
	}
	
	for(i = delaySamples2; i < nSamples; i++) {				
		procBuffer[i] +=  (uint16_t)(origBuffer[i-delaySamples2] * gain2);		
	}

	/* Move data to the original (playback) buffer */
	memcpy((uint8_t *)origBuffer, (uint8_t *)procBuffer, nSamples*sizeof(uint16_t));	
	
	/* Release resources */
	free(procBuffer);	
}

/* **************************************************************
 * Audio processing example:
 *  	Applies a low-pass filter
 * 		Args are cutoff frequency, buffer and nsamples in buffer
 * 
 * 		Simple realization derived from the discretization on an analog RC low-pass filter. See e.g. 
 * 			https://en.wikipedia.org/wiki/Low-pass_filter#Simple_infinite_impulse_response_filter 
 * ************************************************************ */ 
// Funcao para evitar saturacao usada no LP filter
float clampf(float v, float min, float max) {
	if (v < min) return min;
	if (v > max) return max;
	return v;
}

// Funcao do filtro passa baixo alterada
// Agora recebe valores com sinal
 void filterLP(uint32_t cof, uint32_t sampleFreq, uint8_t * buffer, uint32_t nSamples)
{					
	
	int i;
	
	int16_t * origBuffer; 	/* Agora em S16 com sinal */
	
	float alfa, beta; 
	float y, s;

	/* Compute alfa and beta multipliers */
	alfa = (2.0f * (float)M_PI / (float)sampleFreq * (float)cof ) / ( (2.0f * (float)M_PI / (float)sampleFreq * (float)cof ) + 1.0f );
	beta = 1.0f - alfa;
	
	/* Get pointer to buffer of the right type */
	origBuffer = (int16_t *)buffer;
	
	y = (float)origBuffer[0];

	for(i = 0; i < (int)nSamples; i++) {
		y = alfa * (float)origBuffer[i] + beta * y;
		s = clampf(y, -32768.0f, 32767.0f);
		origBuffer[i] = (int16_t)s;
	}	
	
	return;
}


/* *************************************************************************************
 * Audio processing example:
 *      Generates a sine wave. 
 *      Frequency in Hz, durationMS in miliseconds, amplitude 0...0xFFFF, stream buffer 
 * 
 * *************************************************************************************/ 
void genSineU16(uint16_t freq, uint32_t durationMS, uint16_t amp, uint8_t *buffer)
{	
	int i=0, nSamples=0;
		
	float sinArgK = 2*M_PI*freq;				/* Compute once constant part of sin argument, for efficiency */
	
	uint16_t * bufU16 = (uint16_t *)buffer; 	/* UINT16 pointer to buffer for access sample by sample */
	
	nSamples = ((float)durationMS / 1000) * SAMP_FREQ; 	/* Compute how many samples to generate */
			
	/* Generate sine wave */
	for(i = 0; i < nSamples; i++) {
		bufU16[i] = amp/2*(1+sin((sinArgK*i)/SAMP_FREQ));		
	}		
	
	return;
}

/* *************************************************************************************
 * Debug function 
 *      Returns the max and min amplitude of signal in a buffer - uint16 format
 * 
 * *************************************************************************************/ 
void getMaxMinU16(uint8_t * buffer, uint32_t nSamples, uint32_t * max, uint32_t * min)
{	
	int i=0;
		
	uint16_t * origBuffer; 	/* Pointer to original buffer, with right sample type (UINT16 in the case) */
			
	/* Get pointer to buffer of the right type */
	origBuffer = (uint16_t *)buffer;
	
	/* Get max value */
	*max=origBuffer[0];
	*min=*max;
	for(i = 1; i < nSamples; i++) {		
		if(origBuffer[i] > *max)
			*max=origBuffer[i];
		if(origBuffer[i] < *min)
			*min=origBuffer[i];		
	}
	
	return;	
}

// NOTE - Thread Dispatcher function
// Lock e Unlock para evitar race condicions com a callback
void* dispatcher_loop(void* arg) {
	while(dispatcher_run) {

		// Variavel que verifica se buf foi consumido nesta iteração
		int consumed = 0;

		// Temos de bloquear porque vamos mexer nos buffers
		SDL_LockAudioDevice(gRecDev);

		if (bufA.full) {
			// TODO - AquiEnt no futuro vamos copiar para filas por tarefa
			bufA.full = 0;
			blocksdispatched++;
			consumed = 1;
			printf("[DISPATCH] Consumi A (total=%d)\n", blocksdispatched);

		} else if (bufB.full) {
			bufB.full = 0;
			blocksdispatched++;
			consumed = 1;
			printf("[DISPATCH] Consumi B (total=%d)\n", blocksdispatched);
		}

		SDL_UnlockAudioDevice(gRecDev);

		if (!consumed) {
			SDL_Delay(2); // caso n tenha encontrado nenhum full buf - sleep 2ms para n estar sempre a consumir cpu
		}
	}
	return NULL;
}

/* ***************************************
 * Main 
 * ***************************************/
int main(int argc, char ** argv)
{
	/* ****************
	 *  Variables 
	 **************** */
	SDL_AudioDeviceID recordingDeviceId = 0; 	/* Structure with ID of recording device */
	SDL_AudioDeviceID playbackDeviceId = 0; 	/* Structure with ID of playback device */
	SDL_AudioSpec desiredPlaybackSpec;			/* Structure for desired playback attributes (the ones returned may differ) */
	const char * deviceName;					/* Capture device name */
	int index;									/* Device index used to browse audio devices */
	int bytesPerSample;							/* Number of bytes each sample requires. Function of size of sample and # of channels */ 
	int bytesPerSecond;							/* Intuitive. bytes per sample sample * sampling frequency */
	
	
	/* SDL Init */
	if(SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		return 1;
	}

	/* *************************************
	 * Get and open recording device 
	 ************************************* */
	SDL_AudioSpec desiredRecordingSpec;
	/* Defined in SDL_audio.h */
	SDL_zero(desiredRecordingSpec);				/* Init struct with default values */
	desiredRecordingSpec.freq = SAMP_FREQ;		/* Samples per second */
	desiredRecordingSpec.format = FORMAT;		/* Sampling format */
	desiredRecordingSpec.channels = MONO;		/* 1 - mono; 2 stereo */
	desiredRecordingSpec.samples = ABUFSIZE_SAMPLES;		/* Audio buffer size in sample FRAMES (total samples divided by channel count) */
	desiredRecordingSpec.callback = audioRecordingCallback;

	/* Get number of recording devices */
	gRecordingDeviceCount = SDL_GetNumAudioDevices(SDL_TRUE);		/* Argument is "iscapture": 0 to request playback device, !0 for recording device */

	if(gRecordingDeviceCount < 1)
	{
		printf( "Unable to get audio capture device! SDL Error: %s\n", SDL_GetError() );
		return 0;
	}
	
	/* and lists them */
	for(int i = 0; i < gRecordingDeviceCount; ++i)
	{
		//Get capture device name
		deviceName = SDL_GetAudioDeviceName(i, SDL_TRUE);/* Arguments are "index" and "iscapture"*/
		printf("%d - %s\n", i, deviceName);
	}

	/* If device index supplied as arg, use it, otherwise, ask the user */
	if(argc == 2) {
		index = atoi(argv[1]);		
	} else {
		/* allow the user to select the recording device */
		printf("Choose audio\n");
		scanf("%d", &index);
	}
	
	if(index < 0 || index >= gRecordingDeviceCount) {
		printf( "Invalid device ID. Must be between 0 and %d\n", gRecordingDeviceCount-1 );
		return 0;
	} else {
		printf( "Using audio capture device %d - %s\n", index, deviceName );
	}

	/* and open it */
	recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(index, SDL_TRUE), SDL_TRUE, &desiredRecordingSpec, &gReceivedRecordingSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

	gRecDev = recordingDeviceId;

	/* if device failed to open terminate */
	if(recordingDeviceId == 0)
	{
		//Report error
		printf("Failed to open recording device! SDL Error: %s", SDL_GetError() );
		return 1;
	}


	/* **********************************
	 *  Get and open playback 
	 * **********************************/
	
	SDL_zero(desiredPlaybackSpec);
	desiredPlaybackSpec.freq = SAMP_FREQ;
	desiredPlaybackSpec.format = FORMAT; 
	desiredPlaybackSpec.channels = MONO;
	desiredPlaybackSpec.samples = ABUFSIZE_SAMPLES;
	desiredPlaybackSpec.callback = audioPlaybackCallback;

	/* Open playback device */
	playbackDeviceId = SDL_OpenAudioDevice( NULL, SDL_FALSE, &desiredPlaybackSpec, &gReceivedPlaybackSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE );

	/* if error abort */
	if(playbackDeviceId == 0)
	{
		//Report error
		printf("Failed to open playback device! SDL Error: %s", SDL_GetError());
		return 1;
	}


	/* **************************************************
	 * Recording and playback devices opened and OK.
	 * Time to init some data structures 
	 * **************************************************/ 
	/* Calculate number of bytes per sample */
	bytesPerSample = gReceivedRecordingSpec.channels * (SDL_AUDIO_BITSIZE(gReceivedRecordingSpec.format) / 8);

	/* Calculate number of bytes per second */
	bytesPerSecond = gReceivedRecordingSpec.freq * bytesPerSample;

	/* Calculate buffer size, for the desired duration  */
	gBufferByteSize = RECORDING_BUFFER_SECONDS * bytesPerSecond;

	/* Calculate max buffer use - some additional space to allow for extra samples*/
	/* Detection of buffer use is made form device-driver callback, so can be a biffer overrun if some */
	/* leeway is not added */ 
	gBufferByteMaxPosition = MAX_RECORDING_SECONDS * bytesPerSecond;

	/* Allocate and initialize record buffer */
	gRecordingBuffer = (uint8_t *)malloc(gBufferByteSize);
	memset(gRecordingBuffer, 0, gBufferByteSize);
	
	printf("\n\r *********** \n\r");
	printf("bytesPerSample=%d, bytesPerSecond=%d, buffer byte size=%d (allocated) buffer byte size=%d (for nominal recording)", \
			bytesPerSample, bytesPerSecond,gBufferByteSize, gBufferByteMaxPosition);
	printf("\n\r *********** \n\r");

#define RECORD
#ifdef RECORD

	/* ******************************************************
	 * All set. Time to record, process and play sounds  
	 * ******************************************************/
	
	printf("Recording\n");
	
	/* Set index to the beginning of buffer */
	gBufferBytePosition = 0;
	
	// NOTE - Criar a thread do dispacher antes de começar a gravar
	if (pthread_create(&dispatcher_th, NULL, dispatcher_loop, NULL) != 0) {
		perror("pthread_create");
		return 1;
	}

	/* After being open devices have callback processing blocked (paused_on active), to allow configuration without glitches */
	/* Devices must be unpaused to allow callback processing */
	SDL_PauseAudioDevice(recordingDeviceId, SDL_FALSE ); /* Args are SDL device id and pause_on */

	/* Wait until recording buffer full */
	while(1)
	{
		/* Lock callback. Prevents the following code to not concur with callback function */
		SDL_LockAudioDevice(recordingDeviceId);

		/* Receiving buffer full? */
		if(blocksdispatched >= 50)
		{
			/* Stop recording audio */
			SDL_PauseAudioDevice(recordingDeviceId, SDL_TRUE );
			SDL_UnlockAudioDevice(recordingDeviceId );
			break;
		}
		/*if (bufA.full || bufB.full) {
			blocksRecorded++;
		}*/

		/* Buffer not yet full? Keep trying ... */
		SDL_UnlockAudioDevice( recordingDeviceId );

		// REVIEW - Esta parte é para retirar
		// Era apenas para ver se o double buffer estava funcional
		// do ponto de vista de recolha das amostras e alternancia
		// NOTE - teste temporário
		// Simular o comportamento de consumo dos buffers A e B
		// Para verificar se eles de facto ficam cheios alternadamente
		/*if (bufA.full) {
			printf("[BUF] Buffer A cheio\n");
			bufA.full = 0;	// simula processamento dos dados
		}
		if (bufB.full) {
			printf("[BUF] Buffer B cheio\n");
			bufB.full = 0;
		}*/
	}

	// NOTE - variavel de controlo da thread a zero termina a thread
	// pthread join para esperar que a thread acabe em segurança
	dispatcher_run = 0;
	if (pthread_join(dispatcher_th, NULL) != 0) {
		perror("pthread_join");
	}

	/* *****************************************************************
	 * Recorded data obtained. Now process it and play it back
	 * *****************************************************************/
 
#endif

//#define GENSINE
#ifdef GENSINE
	printf("\n Generating a sine wave \n");
	genSineU16(1000, 1000, 30000, gRecordingBuffer); 	/* freq, durationMS, amp, buffer */
#endif

//#define ADDECHO
#ifdef ADDECHO
	printf("Adding an echo \n");
	addEchoU16(1000, 2000, 0.2, 0.1,gRecordingBuffer,gBufferByteMaxPosition/sizeof(uint16_t)); /* (float delay1MS, float delay2MS, float gain1, float gain2, uint8 * buffer, uint32_t nSamples) */
#endif

	/* For debug - if you want to check the data. */
	/* Can be usefull to add a function that dumps the data to a file for processing e.g. in Matlab/Octave */
//	printSamplesU16(gRecordingBuffer,4000); /* Args are pointer to buffer and nsamples (not bytes)	*/		
	
	/* For debug															*/ 
	/* Getting max-min can be usefull to detect possible saturation 		*/
	{
		uint32_t max, min;
		getMaxMinU16(gRecordingBuffer,gBufferByteMaxPosition/sizeof(uint16_t), &max, &min);
		printf("Maxa mplitude: = %u Min amplitude is:%u\n",max, min);
	}


//#define MAXMIN
#ifdef MAXMIN	
	{
		uint32_t max, min;
		getMaxMinU16(gRecordingBuffer, gBufferByteMaxPosition/sizeof(uint16_t), &max, &min);  // getMaxMinU16(uint8_t * buffer, uint32_t nSamplesm, uint32_t max, uint32_t min)
		printf("\n Max amplitude = %u Min amplitude = %u\n",max, min);
	}
#endif	

#define LPFILTER
#ifdef LPFILTER
	/* Apply LP filter */
	/* Args are cutoff freq, sampling freq, buffer and # of samples in the buffer */
	printf("\n Applying LP filter \n");
	filterLP(1000, SAMP_FREQ, gRecordingBuffer, gBufferByteMaxPosition/sizeof(uint16_t)); 
#endif

#define FFT
#ifdef FFT
	{		
		int N=0;	// Number of samples to take
		int sampleDurationMS = 100; /* Duration of the sample to analyze, in ms */
		int k=0; 	// General counter
		uint16_t * sampleVector = (uint16_t *)gRecordingBuffer; // Vector of samples 
		float * fk; /* Pointer to array with frequencies */
		float * Ak; /* Pointer to array with amplitude for frequency fk[i] */
		complex double * x; /* Pointer to array of complex values for samples and FFT */
	
		printf("\nComputing FFT of signal\n");
		
		/* Get the vector size, making sure it is a power of two */
		for(N=1; pow(2,N) < (SAMP_FREQ*sampleDurationMS)/1000; N++);
		N--;
		N=(int)pow(2,N);
		
		printf("# of samples is: %d\n",N);
		
		/* Allocate memory for  samples, frequency and amplitude vectors */
		x = (complex double *)malloc(N * sizeof(complex double)); /* Array for samples and FFT output */
		fk = (float *)malloc(N * sizeof(float)); 	/* Array with frequencies */
		Ak = (float *)malloc(N * sizeof(float)); 	/* Array with amplitude for frequency fk[i] */
				
		/* Copy samples to complex input vector */
		for(k=0; k<N;k++) {
			x[k] = sampleVector[k];
		}
				
		/* Compute FFT */
		fftCompute(x, N);

		//printf("\nFFT result:\n");/		
		//printComplexArray(x, N);
    
		/* Compute the amplitude at each frequency and print it */
		fftGetAmplitude(x,N,SAMP_FREQ, fk,Ak);

		for(k=0; k<=N/2; k++) {
			printf("Amplitude at frequency %f Hz is %f \n", fk[k], Ak[k]);
		}
	}
	
#endif
	
	/* *****************************************************************
	 * Recorded/generated data obtained. Now play it back
	 * *****************************************************************/
	printf("Playback\n");
		
	/* Reset buffer index to the beginning */
	gBufferBytePosition = 0;
	
	/* Enable processing of callbacks by playback device (required after opening) */
	SDL_PauseAudioDevice(playbackDeviceId, SDL_FALSE);

	/* Play buffer */
	while(1)
	{
		/* Lock callback */
		SDL_LockAudioDevice(playbackDeviceId);

		/* Playback is finished? */
		if(gBufferBytePosition > gBufferByteMaxPosition)
		{
			/* Stop playing audio */
			SDL_PauseAudioDevice(playbackDeviceId, SDL_TRUE);
			SDL_UnlockAudioDevice(playbackDeviceId);	
			break;
		}

		/* Unlock callback and try again ...*/
		SDL_UnlockAudioDevice(playbackDeviceId);
	}

	/* *******************************************
	 * All done! Release resources and terminate
	 * *******************************************/
	if( gRecordingBuffer != NULL )
	{
		free(gRecordingBuffer);
		gRecordingBuffer = NULL;
	}

	SDL_Quit();
	
	return 0;
}
