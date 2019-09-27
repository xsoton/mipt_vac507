#include "main.h"

#ifdef LOMO
void data_lomo_written_wait(params_t *p)
{
	pthread_mutex_lock(&p->data_lomo_written_mutex);
		pthread_cond_wait(&p->data_lomo_written_cond, &p->data_lomo_written_mutex);
	pthread_mutex_unlock(&p->data_lomo_written_mutex);
}

void data_lomo_written_signal(params_t *p)
{
	pthread_mutex_lock(&p->data_lomo_written_mutex);
		pthread_cond_signal(&p->data_lomo_written_cond);
	pthread_mutex_unlock(&p->data_lomo_written_mutex);
}

void data_lomo_written_broadcast(params_t *p)
{
	pthread_mutex_lock(&p->data_lomo_written_mutex);
		pthread_cond_broadcast(&p->data_lomo_written_cond);
	pthread_mutex_unlock(&p->data_lomo_written_mutex);
}
#endif
