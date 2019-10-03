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
