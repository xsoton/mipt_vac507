#include "main.h"

void data_ready_wait(params_t *p)
{
	pthread_mutex_lock(&p->data_ready_mutex);
		pthread_cond_wait(&p->data_ready_cond, &p->data_ready_mutex);
	pthread_mutex_unlock(&p->data_ready_mutex);
}

void data_ready_signal(params_t *p)
{
	pthread_mutex_lock(&p->data_ready_mutex);
		pthread_cond_signal(&p->data_ready_cond);
	pthread_mutex_unlock(&p->data_ready_mutex);
}
