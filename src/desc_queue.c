#include <string.h>
#include "desc_queue.h"


// Funcao de inicialização das filas de descritores do dispatcher
// Colocar tudo a zero e inicializar mtx
void desc_queue_init(DescQueue *q)
{
    memset(q, 0, sizeof(*q));
    pthread_mutex_init(&q->mtx, NULL);
}

// Funcao de push das filas do dispatcher
// Recebe ponteiro para a queue a adicionar o descritor
// E também o descritor a adicionar
int desc_queue_push(DescQueue *q, AudioDesc d)
{

	pthread_mutex_lock(&q->mtx);

	// Ver se a fila ja esta cheia
	// Se sim descartamos o mais antigo
	if (q->count == DESCRIPTOR_QUEUE_CAPACITY)
	{
		q->tail = (q->tail + 1) % DESCRIPTOR_QUEUE_CAPACITY;
		q->count--;
	}

	q->desc[q->head] = d;
	q->head = (q->head + 1) % DESCRIPTOR_QUEUE_CAPACITY;
	q->count++;

	pthread_mutex_unlock(&q->mtx);

	return 1;
}

// Funcao para fazer pop nas filas static de descritores
// Parametro out para a thread depois ter acesso aos dados do descritor
int desc_queue_pop(DescQueue *q, AudioDesc *out)
{

	int ok = 0;
	pthread_mutex_lock(&q->mtx);

	// Se tiver info descritor a processar
	if (q->count > 0)
	{
		*out = q->desc[q->tail];
		q->tail = (q->tail + 1) % DESCRIPTOR_QUEUE_CAPACITY;
		q->count--;
		ok = 1;
	}

	pthread_mutex_unlock(&q->mtx);
	
	return ok;
}