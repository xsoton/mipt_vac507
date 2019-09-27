#include "main.h"

#ifdef LOMO
void *worker_lomo(void *arg)
{
	params_t *p = (params_t *)arg;
	int r;

	int lomo_fd;
	FILE *fp;

	int index_lomo = 0;
	double adc = 0.0;

	r = lomo_open(LOMO_TTY, &lomo_fd);
		ERR(r < 0, worker_lomo_exit, "# E: Unable to open lomo (%d)\n", r);
	r = lomo_init(lomo_fd);
		ERR(r < 0, worker_lomo_close, "# E: Unable to init lomo (%d)\n", r);

	fp = fopen(p->filename_lomo, "w+");
		ERR(fp == NULL, worker_lomo_close, "# E: Unable to open file \"%s\" (%s)\n", p->filename_lomo, strerror(ferror(fp)));

	setlinebuf(fp);

	int col = 1;
	r = fprintf(fp, "# %d: lomo index\n", col++);
		ERR(r < 0, writer_lomo_fp_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_lomo, strerror(r));
	r = fprintf(fp, "# %d: lomo adc, a.u. [0-1]\n", col++);
		ERR(r < 0, writer_lomo_fp_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_lomo, strerror(r));

	while(get_run(p))
	{
		r = lomo_read_value(lomo_fd, &adc);
			ERR(r < 0, worker_lomo_end_loop, "# E: Unable to read lomo adc (%d)\n", r);

		r = fprintf(fp, "%d\t%le\n", index_lomo, adc);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_lomo, strerror(r));
			set_run(p, 0);
		}

		worker_lomo_end_loop:

		pthread_rwlock_wrlock(&p->data_lock);
			p->index_lomo = index_lomo;
			p->ready_lomo = 1;
			p->adc = adc;
		pthread_rwlock_unlock(&p->data_lock);

		data_lomo_written_signal(p);

		index_lomo++;
	}

	writer_lomo_fp_close:

	fclose(fp);

	worker_lomo_close:
	r = lomo_close(lomo_fd);
		ERR(r < 0, worker_lomo_exit, "# E: Unable to close lomo (%d)\n", r);

	worker_lomo_exit:
	return NULL;
}
#endif
