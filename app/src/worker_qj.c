#include "main.h"

#ifdef QJ
void *worker_qj(void *arg)
{
	exp_t *p = (exp_t *)arg;
	int r;

	int qj_fd;
	FILE *fp;

	int index = 0;
	double voltage = 0.0;
	double current = 0.0;

	r = qj3003p_open(QJ3003P_TTY, &qj_fd);
		ERR(r < 0, worker_qj_exit, "# E: Unable to open qj (%d)\n", r);
	r = qj3003p_set_output(qj_fd, 0);
		ERR(r < 0, worker_qj_deinit, "# E: Unable to set qj output (%d)\n", r);
	r = qj3003p_set_voltage(qj_fd, 0.0);
		ERR(r < 0, worker_qj_deinit, "# E: Unable to set qj voltage (%d)\n", r);
	r = qj3003p_set_current(qj_fd, 0.5);
		ERR(r < 0, worker_qj_deinit, "# E: Unable to set qj current (%d)\n", r);
	r = qj3003p_set_output(qj_fd, 1);
		ERR(r < 0, worker_qj_deinit, "# E: Unable to set qj output (%d)\n", r);

	fp = fopen(p->filename_qj, "w+");
		ERR(fp == NULL, worker_qj_deinit, "# E: Unable to open file \"%s\" (%s)\n", p->filename_qj, strerror(ferror(fp)));

	setlinebuf(fp);

	int col = 1;
	r = fprintf(fp, "# %d: qj index\n", col++);
		ERR(r < 0, writer_qj_fp_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_qj, strerror(r));
	r = fprintf(fp, "# %d: pps U, V\n", col++);
		ERR(r < 0, writer_qj_fp_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_qj, strerror(r));
	r = fprintf(fp, "# %d: pps I, A\n", col++);
		ERR(r < 0, writer_qj_fp_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_qj, strerror(r));

	while(get_run(p))
	{
		if ((index + 1) * U_STEP > 10)
		{
			set_run(p, 0);
			goto worker_qj_end_loop;
		}

		r = qj3003p_set_voltage(qj_fd, index * U_STEP);
			ERR(r < 0, worker_qj_end_loop, "# E: Unable to set qj voltage (%d)\n", r);

		sleep(TIME_DELAY_S);

		r = qj3003p_get_voltage(qj_fd, &voltage);
			ERR(r < 0, worker_qj_end_loop, "# E: Unable to get qj voltage (%d)\n", r);
		r = qj3003p_get_current(qj_fd, &current);
			ERR(r < 0, worker_qj_end_loop, "# E: Unable to get qj current (%d)\n", r);

		r = fprintf(fp, "%d\t%le\t%le\n", index, voltage, current);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_qj, strerror(r));
			set_run(p, 0);
		}

		worker_qj_end_loop:

		pthread_rwlock_wrlock(&p->data_lock);
			p->index = index;
			p->ready_qj = 1;
			p->voltage = voltage;
			p->current = current;
		pthread_rwlock_unlock(&p->data_lock);

		data_qj_written_signal(p);

		index++;
	}

	writer_qj_fp_close:

	fclose(fp);

	worker_qj_deinit:
	r = qj3003p_set_voltage(qj_fd, 0);
		ERR(r < 0, worker_qj_close, "# E: Unable to set qj voltage (%d)\n", r);
	r = qj3003p_set_output(qj_fd, 0);
		ERR(r < 0, worker_qj_close, "# E: Unable to set qj voltage (%d)\n", r);

	worker_qj_close:
	r = qj3003p_close(qj_fd);
		ERR(r < 0, worker_qj_exit, "# E: Unable to close qj (%d)\n", r);

	worker_qj_exit:
	return NULL;
}
#endif
