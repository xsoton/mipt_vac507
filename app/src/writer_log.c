#include "main.h"

void *writer(void *arg)
{
	exp_t *p = (exp_t *)arg;
	int r;

	FILE *fp;

	int index = 0;

	#ifdef QJ
		int index_qj;
		int ready_qj;
		double pps_U;
		double pps_I;
	#endif
	#ifdef LOMO
		int index_lomo;
		int ready_lomo;
		double adc;
	#endif
	#ifdef APPA
		int index_appa;
		int ready_appa;
		double mult_value;
		unit_t mult_unit;
		int mult_overload;
	#endif

	fp = fopen(p->filename_log, "w+");
		ERR(fp == NULL, writer_exit, "# E: Unable to open file \"%s\" (%s)\n", p->filename_log, strerror(ferror(fp)));

	setlinebuf(fp);

	int col = 1;
	r = fprintf(fp, "# %d: index\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
#ifdef QJ
	r = fprintf(fp, "# %d: qj index\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
	r = fprintf(fp, "# %d: pps U, V\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
	r = fprintf(fp, "# %d: pps I, A\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
#endif
#ifdef LOMO
	r = fprintf(fp, "# %d: lomo index\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
	r = fprintf(fp, "# %d: lomo adc, a.u. [0-1]\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
#endif
#ifdef APPA
	r = fprintf(fp, "# %d: appa index\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
	r = fprintf(fp, "# %d: appa value, V\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
	r = fprintf(fp, "# %d: appa overload, bool\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
#endif

	while(get_run(p))
	{
		int ready = 1;
		pthread_rwlock_wrlock(&p->data_lock);
			#ifdef QJ
				index_qj = p->index_qj;
				ready_qj = p->ready_qj;
				pps_U = p->pps_U;
				pps_I = p->pps_I;
				ready = ready && ready_qj;
			#endif
			#ifdef LOMO
				index_lomo = p->index_lomo;
				ready_lomo = p->ready_lomo;
				adc = p->adc;
				ready = ready && ready_lomo;
			#endif
			#ifdef APPA
				index_appa = p->index_appa;
				ready_appa = p->ready_appa;
				mult_value = p->mult_value;
				mult_unit = p->mult_unit;
				mult_overload = p->mult_overload;
				ready = ready && ready_appa;
			#endif

			if (ready)
			{
				#ifdef QJ
					p->ready_qj = 0;
				#endif
				#ifdef LOMO
					p->ready_lomo = 0;
				#endif
				#ifdef APPA
					p->ready_appa = 0;
				#endif
			}
		pthread_rwlock_unlock(&p->data_lock);

		if (!ready)
		{
			usleep(200000);
			goto writer_end_loop;
		}

		r = fprintf(fp,
			"%d"
#ifdef QJ
			"\t%d"
			"\t%le"
			"\t%le"
#endif
#ifdef LOMO
			"\t%d"
			"\t%le"
#endif
#ifdef APPA
			"\t%d"
			"\t%le"
			"\t%d"
#endif
			"\n"
			, index
#ifdef QJ
			, index_qj
			, pps_U
			, pps_I
#endif
#ifdef LOMO
			, index_lomo
			, adc
#endif
#ifdef APPA
			, index_appa
			, mult_value
			, mult_overload
#endif
		);

		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to print to file \"%s\" (%s)\n", p->filename_log, strerror(r));
			set_run(p, 0);
		}

		index++;

		writer_end_loop:

		data_log_written_signal(p);
	}

writer_close:

	fclose(fp);

writer_exit:

	return NULL;
}
