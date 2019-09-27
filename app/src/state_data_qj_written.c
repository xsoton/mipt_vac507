#include "main.h"

#ifdef QJ
void data_qj_written_wait(params_t *p)
{
	pthread_mutex_lock(&p->data_qj_written_mutex);
		pthread_cond_wait(&p->data_qj_written_cond, &p->data_qj_written_mutex);
	pthread_mutex_unlock(&p->data_qj_written_mutex);
}

void data_qj_written_signal(params_t *p)
{
	pthread_mutex_lock(&p->data_qj_written_mutex);
		pthread_cond_signal(&p->data_qj_written_cond);
	pthread_mutex_unlock(&p->data_qj_written_mutex);
}

void data_qj_written_broadcast(params_t *p)
{
	pthread_mutex_lock(&p->data_qj_written_mutex);
		pthread_cond_broadcast(&p->data_qj_written_cond);
	pthread_mutex_unlock(&p->data_qj_written_mutex);
}
#endif
