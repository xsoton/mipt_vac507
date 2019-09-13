#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "lomo.h"
#include "appa208.h"
#include "qj3003p.h"

#define QJ
#define LOMO
// #define APPA

#define LOMO_TTY "/dev/ttyUSB2"
#define QJ3003P_TTY "/dev/ttyUSB0"
#define APPA208_TTY "/dev/ttyUSB1"

#define U_STEP 0.1
#define TIME_DELAY_S 10

#define DBG(s)
#define DBGP(s, ...)
// #define DBG(s) do{fprintf(stderr,s);}while(0);
// #define DBGP(s, ...) do{fprintf(stderr,s,__VA_ARGS__);}while(0);
#define ERR(c, g, s, ...) do{if(c){fprintf(stderr,s,__VA_ARGS__);set_run(p, 0);goto g;}}while(0);
#define ERR_PL(c, g, s, ...) do{if(c){fprintf(stderr,s,__VA_ARGS__);set_run(p, 0);data_ready_signal(p);goto g;}}while(0);

typedef struct
{
	pthread_rwlock_t run_lock;
	int run;
	pthread_mutex_t eq_init_mutex;
	pthread_cond_t eq_init_cond;
	pthread_mutex_t data_ready_mutex;
	pthread_cond_t data_ready_cond;
	pthread_mutex_t data_written_mutex;
	pthread_cond_t data_written_cond;
	char filename[100];
	int index;
#ifdef QJ
	double pps_U;
	double pps_I;
#endif
#ifdef LOMO
	double adc;
#endif
#ifdef APPA
	double mult_value;
	unit_t mult_unit;
	int mult_overload;
#endif
} params_t;

void *commander(void *arg);
void *worker(void *arg);
void *writer(void *arg);
void *plotter(void *arg);

int get_run(params_t *p);
void set_run(params_t *p, int run);

void eq_init_wait(params_t *p);
void eq_init_signal(params_t *p);

void data_ready_wait(params_t *p);
void data_ready_signal(params_t *p);

void data_written_wait(params_t *p);
void data_written_signal(params_t *p);
void data_written_broadcast(params_t *p);

int main(int argc, char const *argv[])
{
	int ret = 0;

	params_t params = {0};
	pthread_rwlock_init(&params.run_lock, NULL);
	pthread_mutex_init(&params.eq_init_mutex, NULL);
	pthread_cond_init(&params.eq_init_cond, NULL);
	pthread_mutex_init(&params.data_ready_mutex, NULL);
	pthread_cond_init(&params.data_ready_cond, NULL);
	pthread_mutex_init(&params.data_written_mutex, NULL);
	pthread_cond_init(&params.data_written_cond, NULL);
	params.run = 1;

	pthread_t t_commander;
	pthread_t t_worker;
	pthread_t t_writer;
	pthread_t t_plotter;

	if (argc < 2)
	{
		fprintf(stderr, "# E: Usage: vac <filename.dat>\n");
		ret = -1;
		goto main_exit;
	}

	setlinebuf(stdout);
	setlinebuf(stderr);

	strncpy(params.filename, argv[1], 100);

	pthread_create(&t_commander, NULL, commander, &params);
	pthread_create(&t_worker, NULL, worker, &params);
	pthread_create(&t_writer, NULL, writer, &params);
	pthread_create(&t_plotter, NULL, plotter, &params);

	pthread_join(t_worker, NULL);
	pthread_join(t_writer, NULL);
	pthread_join(t_plotter, NULL);

	pthread_cancel(t_commander);
	pthread_join(t_commander, NULL);

main_exit:

	return ret;
}

void *commander(void *arg)
{
	params_t *p = (params_t *)arg;
	char str[100];
	char *s;

	while(get_run(p))
	{
		printf("> ");

		s = fgets(str, 100, stdin);
		if (s == NULL)
		{
			fprintf(stderr, "# E: Exit\n");
			set_run(p, 0);
			break;
		}

		switch(str[0])
		{
			int ccount;
			case 'h':
				printf(
					"Help:\n"
					"\th -- this help;\n"
					"\tq -- exit the program;\n");
				break;
			case 'q':
				set_run(p, 0);
				break;
			default:
				ccount = strlen(str)-1;
				fprintf(stderr, "# E: Unknown commang (%.*s)\n", ccount, str);
				break;
		}
	}

	return NULL;
}

void *worker(void *arg)
{
	params_t *p = (params_t *)arg;
	int r;

#ifdef LOMO
	int lomo_fd;
#endif
#ifdef APPA
	int appa_fd;
#endif
#ifdef QJ
	int qj_fd;
#endif

	double step_U = U_STEP;

#ifdef APPA
	appa208_disp_t disp;
#endif

#ifdef LOMO
DBG("# D: lomo open\n");
	r = lomo_open(LOMO_TTY, &lomo_fd);
		ERR(r < 0, worker_exit, "# E: Unable to open lomo (%d)\n", r);
DBG("# D: lomo init\n");
	r = lomo_init(lomo_fd);
		ERR(r < 0, worker_close_lomo, "# E: Unable to init lomo (%d)\n", r);
#endif

#ifdef APPA
DBG("# D: appa open\n");
	r = appa208_open(APPA208_TTY, &appa_fd);
		ERR(r < 0, worker_close_lomo, "# E: Unable to open appa (%d)\n", r);
DBG("# D: appa read\n");
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
		ERR(r < 0, worker_close_appa, "# E: Unable to read appa display (%d)\n", r);
	p->mult_unit = appa208_get_unit(&disp.mdata);
#endif

#ifdef QJ
DBG("# D: qj open\n");
	r = qj3003p_open(QJ3003P_TTY, &qj_fd);
		ERR(r < 0, worker_close_appa, "# E: Unable to open qj (%d)\n", r);
DBG("# D: qj set output\n");
	r = qj3003p_set_output(qj_fd, 0);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj output (%d)\n", r);
DBG("# D: qj set voltage\n");
	r = qj3003p_set_voltage(qj_fd, 0.0);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj voltage (%d)\n", r);
DBG("# D: qj set current\n");
	r = qj3003p_set_current(qj_fd, 3.0);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj current (%d)\n", r);
DBG("# D: qj set output\n");
	r = qj3003p_set_output(qj_fd, 1);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj output (%d)\n", r);
#endif

	eq_init_signal(p);

	while(get_run(p))
	{
		if ((p->index + 1) * step_U > 10)
		{
DBG("# D: exit\n");
			set_run(p, 0);
			goto worker_end_loop;
		}

DBGP("next_read %d\n", p->index);

#ifdef QJ
		r = qj3003p_set_voltage(qj_fd, p->index * step_U);
			ERR(r < 0, worker_end_loop, "# E: Unable to set qj voltage (%d)\n", r);

		sleep(TIME_DELAY_S);

		r = qj3003p_get_voltage(qj_fd, &p->pps_U);
			ERR(r < 0, worker_end_loop, "# E: Unable to get qj voltage (%d)\n", r);
		r = qj3003p_get_current(qj_fd, &p->pps_I);
			ERR(r < 0, worker_end_loop, "# E: Unable to get qj current (%d)\n", r);
#endif

#ifdef LOMO
		r = lomo_read_value(lomo_fd, &p->adc);
			ERR(r < 0, worker_end_loop, "# E: Unable to read lomo adc (%d)\n", r);
#endif

#ifdef APPA
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
			ERR(r < 0, worker_end_loop, "# E: Unable to read appa display (%d)\n", r);
		p->mult_value = appa208_get_value(&disp.mdata);
		p->mult_unit = appa208_get_unit(&disp.mdata);
		p->mult_overload = appa208_get_overload(&disp.mdata);
#endif

worker_end_loop:

		data_ready_signal(p);

		data_written_wait(p);

		p->index++;
	}

worker_exit_qj:

#ifdef QJ
	r = qj3003p_set_voltage(qj_fd, 0);
		ERR(r < 0, worker_close_qj, "# E: Unable to set qj voltage (%d)\n", r);
	r = qj3003p_set_output(qj_fd, 0);
		ERR(r < 0, worker_close_qj, "# E: Unable to set qj voltage (%d)\n", r);
#endif

worker_close_qj:

#ifdef QJ
	r = qj3003p_close(qj_fd);
		ERR(r < 0, worker_close_appa, "# E: Unable to close qj (%d)\n", r);
#endif

worker_close_appa:

#ifdef APPA
	r = appa208_close(appa_fd);
		ERR(r < 0, worker_close_lomo, "# E: Unable to close appa (%d)\n", r);
#endif

worker_close_lomo:

#ifdef LOMO
	r = lomo_close(lomo_fd);
		ERR(r < 0, worker_exit, "# E: Unable to close lomo (%d)\n", r);
#endif

worker_exit:

	return NULL;
}

void *writer(void *arg)
{
	params_t *p = (params_t *)arg;
	int r;

	FILE *fp;

	fp = fopen(p->filename, "w+");
		ERR(fp == NULL, writer_exit, "# E: Unable to open file \"%s\" (%s)\n", p->filename, strerror(ferror(fp)));

	setlinebuf(fp);

	eq_init_wait(p);

	int col = 1;
	r = fprintf(fp, "# %d: index\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename, strerror(r));
#ifdef QJ
	r = fprintf(fp, "# %d: pps U, V\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename, strerror(r));
	r = fprintf(fp, "# %d: pps I, A\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename, strerror(r));
#endif
#ifdef LOMO
	r = fprintf(fp, "# %d: lomo adc, a.u.\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename, strerror(r));
#endif
#ifdef APPA
	r = fprintf(fp, "# %d: appa value, %s\n", col++, appa208_str_unit(p->mult_unit));
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename, strerror(r));
	r = fprintf(fp, "# %d: appa overload, bool\n", col++);
		ERR(r < 0, writer_close, "# E: Unable to print to file \"%s\" (%s)\n", p->filename, strerror(r));
#endif

	while(get_run(p))
	{
		data_ready_wait(p);

DBGP("next_write %d\n", p->index);

		r = fprintf(fp,
			"%d"
#ifdef QJ
			"\t%le"
			"\t%le"
#endif
#ifdef LOMO
			"\t%le"
#endif
#ifdef APPA
			"\t%le"
			"\t%d"
#endif
			"\n"
			, p->index
#ifdef QJ
			, p->pps_U
			, p->pps_I
#endif
#ifdef LOMO
			, p->adc
#endif
#ifdef APPA
			, p->mult_value
			, p->mult_overload
#endif
		);

		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to print to file \"%s\" (%s)\n", p->filename, strerror(r));
			set_run(p, 0);
		}

		data_written_broadcast(p);
	}

writer_close:

	fclose(fp);

writer_exit:

	return NULL;
}

void *plotter(void *arg)
{
	params_t *p = (params_t *)arg;

	FILE *gp = NULL;

	const char *gp_cmd = "gnuplot 2>/dev/null";

	gp = popen(gp_cmd, "w");
	if (gp == NULL)
	{
		fprintf(stderr, "# E: Unable to open pipe \"%s\"\n", gp_cmd);
		set_run(p, 0);
		goto plotter_exit;
	}

	setlinebuf(gp);

	fprintf(gp, "set xlabel \"Voltage, V\"\n");
	fprintf(gp, "set ylabel \"ADC signal [0-1]\"\n");
	fprintf(gp, "set xrange [0:10]\n");
	fprintf(gp, "set yrange [0:]\n");

	while(get_run(p))
	{
		data_written_wait(p);

		fprintf(gp, "plot \"%s\" u 2:4 w l\n", p->filename);
	}

	pclose(gp);

plotter_exit:

	return NULL;
}

int get_run(params_t *p)
{
	int run;
	pthread_rwlock_rdlock(&p->run_lock);
		run = p->run;
	pthread_rwlock_unlock(&p->run_lock);
	return run;
}

void set_run(params_t *p, int run)
{
	pthread_rwlock_wrlock(&p->run_lock);
		p->run = run;
	pthread_rwlock_unlock(&p->run_lock);
}

void eq_init_wait(params_t *p)
{
	pthread_mutex_lock(&p->eq_init_mutex);
		pthread_cond_wait(&p->eq_init_cond, &p->eq_init_mutex);
	pthread_mutex_unlock(&p->eq_init_mutex);
}

void eq_init_signal(params_t *p)
{
	pthread_mutex_lock(&p->eq_init_mutex);
		pthread_cond_signal(&p->eq_init_cond);
	pthread_mutex_unlock(&p->eq_init_mutex);
}

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

void data_written_wait(params_t *p)
{
	pthread_mutex_lock(&p->data_written_mutex);
		pthread_cond_wait(&p->data_written_cond, &p->data_written_mutex);
	pthread_mutex_unlock(&p->data_written_mutex);
}

void data_written_signal(params_t *p)
{
	pthread_mutex_lock(&p->data_written_mutex);
		pthread_cond_signal(&p->data_written_cond);
	pthread_mutex_unlock(&p->data_written_mutex);
}

void data_written_broadcast(params_t *p)
{
	pthread_mutex_lock(&p->data_written_mutex);
		pthread_cond_broadcast(&p->data_written_cond);
	pthread_mutex_unlock(&p->data_written_mutex);
}