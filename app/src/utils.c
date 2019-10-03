#include "main.h"

int get_run(exp_t *p)
{
	int run;
	pthread_rwlock_rdlock(&p->run_lock);
		run = p->run;
	pthread_rwlock_unlock(&p->run_lock);
	return run;
}

void set_run(exp_t *p, int run)
{
	pthread_rwlock_wrlock(&p->run_lock);
		p->run = run;
	pthread_rwlock_unlock(&p->run_lock);
}

double get_time()
{
	static int first = 1;
	static struct timeval t_first = {0};
	struct timeval t = {0};
	double ret;

	if (first == 1)
	{
		gettimeofday(&t_first, NULL);
		ret = 0.0;
		first = 0;
	}
	else
	{
		gettimeofday(&t, NULL);
		ret = (t.tv_sec - t_first.tv_sec) * 1e6 + (t.tv_usec - t_first.tv_usec);
		ret /= 1e6;
	}

	return ret;
}