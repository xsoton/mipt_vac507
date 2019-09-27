#include "main.h"

#ifdef APPA
void data_appa_written_wait(params_t *p)
{
	pthread_mutex_lock(&p->data_appa_written_mutex);
		pthread_cond_wait(&p->data_appa_written_cond, &p->data_appa_written_mutex);
	pthread_mutex_unlock(&p->data_appa_written_mutex);
}

void data_appa_written_signal(params_t *p)
{
	pthread_mutex_lock(&p->data_appa_written_mutex);
		pthread_cond_signal(&p->data_appa_written_cond);
	pthread_mutex_unlock(&p->data_appa_written_mutex);
}

void data_appa_written_broadcast(params_t *p)
{
	pthread_mutex_lock(&p->data_appa_written_mutex);
		pthread_cond_broadcast(&p->data_appa_written_cond);
	pthread_mutex_unlock(&p->data_appa_written_mutex);
}
#endif
