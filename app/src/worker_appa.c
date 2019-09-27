#include "main.h"

#ifdef APPA
void *worker_appa(void *arg)
{
	params_t *p = (params_t *)arg;
	int r;

	int appa_fd;
	FILE *fp;

	appa208_disp_t disp;
	int index_appa = 0;
	double mult_value = 0.0;
	unit_t mult_unit = U_None;
	int mult_overload = 0;

	r = appa208_open(APPA208_TTY, &appa_fd);
		ERR(r < 0, worker_appa_exit, "# E: Unable to open appa (%d)\n", r);

	fp = fopen(p->filename_appa, "w+");
		ERR(fp == NULL, writer_appa_close, "# E: Unable to open file \"%s\" (%s)\n", p->filename_appa, strerror(ferror(fp)));

	setlinebuf(fp);

	r = fprintf(fp, "# %d: appa index\n", col++);
		ERR(r < 0, writer_appa_fp_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_appa, strerror(r));
	r = fprintf(fp, "# %d: appa value, V\n", col++);
		ERR(r < 0, writer_appa_fp_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_appa, strerror(r));
	r = fprintf(fp, "# %d: appa overload, bool\n", col++);
		ERR(r < 0, writer_appa_fp_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_appa, strerror(r));

	while(get_run(p))
	{
		// dirty hack
		int i = 0;
		do
		{
			r = appa208_read_disp(appa_fd, &disp);
			if (r == 0)
			{
				break;
			}
			fprintf(stderr, "# W: appa problem (%d)\n", r);
			i++;
		} while (i < 3);
			ERR(r < 0, worker_appa_end_loop, "# E: Unable to read appa display (%d)\n", r);

		mult_value = appa208_get_value(&disp.mdata);
		mult_unit = appa208_get_unit(&disp.mdata);
		mult_overload = appa208_get_overload(&disp.mdata);

		r = fprintf(fp, "%d\t%le\t%d\n", index_appa, mult_value, mult_overload);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_appa, strerror(r));
			set_run(p, 0);
		}

		worker_appa_end_loop:

		pthread_rwlock_wrlock(&p->data_lock);
			p->index_appa = index_appa;
			p->ready_appa = 1;
			p->mult_value = mult_value;
			p->mult_unit = mult_unit;
			p->mult_overload = mult_overload;
		pthread_rwlock_unlock(&p->data_lock);

		data_appa_written_signal(p);

		index_appa++;
	}

	writer_appa_fp_close:

	fclose(fp);

	writer_appa_close:

	r = appa208_close(appa_fd);
		ERR(r < 0, worker_appa_exit, "# E: Unable to close appa (%d)\n", r);

	worker_appa_exit:
	return NULL;
}
#endif
