#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "lomo.h"
#include "appa208.h"
#include "qj3003p.h"

#define LOMO_TTY "/dev/ttyUSB2"
#define QJ3003P_TTY "/dev/ttyUSB0"
#define APPA208_TTY "/dev/ttyUSB1"

typedef struct
{
	pthread_rwlock_t run_lock;
	int run;
	pthread_mutex_t plot_mutex;
	pthread_cond_t plot_cond;
	char filename[100];
} params_t;

#define PARAMS_INIT {PTHREAD_RWLOCK_INITIALIZER, 1, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, {0}}

void *commander(void *arg);
void *worker(void *arg);
void *plotter(void *arg);

int get_run(params_t *p);
void set_run(params_t *p, int run);

void plot_wait(params_t *p);
void plot_signal(params_t *p);

int main(int argc, char const *argv[])
{
	int ret = 0;
	// int r;

	params_t params = PARAMS_INIT;

	pthread_t t_commander;
	pthread_t t_worker;
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
	pthread_create(&t_plotter, NULL, plotter, &params);

	pthread_join(t_worker, NULL);
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

#define LOMO
#define APPA
// #define QJ

void *worker(void *arg)
{
	params_t *p = (params_t *)arg;
	int r;

	FILE *fp;
#ifdef LOMO
	int lomo_fd;
#endif
#ifdef APPA
	int appa_fd;
#endif
#ifdef QJ
	int qj_fd;
#endif

	int index = 0;
	double step_U = 0.1;

#ifdef LOMO
	double adc;
#endif
#ifdef APPA
	appa208_disp_t disp;
	double mult_value;
	unit_t mult_unit;
	int mult_overload;
#endif
#ifdef QJ
	double pps_U;
	double pps_I;
#endif


#define ERR(c, g, s, ...) do{if(c){fprintf(stderr,s,__VA_ARGS__);set_run(p, 0);goto g;}}while(0);

// printf("01\n");

#ifdef LOMO
	r = lomo_open(LOMO_TTY, &lomo_fd);
		ERR(r < 0, worker_exit, "# E: Unable to open lomo (%d)", r);
// printf("011\n");
	r = lomo_init(lomo_fd);
		ERR(r < 0, worker_close_lomo, "# E: Unable to init lomo (%d)", r);
#endif

// printf("02\n");
#ifdef APPA
	r = appa208_open(APPA208_TTY, &appa_fd);
		ERR(r < 0, worker_close_lomo, "# E: Unable to open appa (%d)", r);
	r = appa208_read_disp(appa_fd, &disp);
		ERR(r < 0, worker_close_appa, "# E: Unable to read appa display (%d)", r);
	mult_unit = appa208_get_unit(&disp.mdata);
#endif

// printf("03\n");

#ifdef QJ
	r = qj3003p_open(QJ3003P_TTY, &qj_fd);
		ERR(r < 0, worker_close_appa, "# E: Unable to open qj (%d)\n", r);
	r = qj3003p_set_output(qj_fd, 0);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj output (%d)\n", r);
	r = qj3003p_set_voltage(qj_fd, 0.0);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj voltage (%d)\n", r);
	r = qj3003p_set_current(qj_fd, 3.0);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj current (%d)\n", r);
	r = qj3003p_set_output(qj_fd, 1);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj output (%d)\n", r);
#endif


	fp = fopen(p->filename, "w+");
		ERR(fp == NULL, worker_exit_qj, "# E: Unable to open file \"%s\"\n", p->filename);

	setlinebuf(fp);

	int col = 1;
	fprintf(fp, "# %d: index\n", col++);
#ifdef QJ
	fprintf(fp, "# %d: pps U, V\n", col++);
	fprintf(fp, "# %d: pps I, A\n", col++);
#endif
#ifdef LOMO
	fprintf(fp, "# %d: lomo adc, a.u.\n", col++);
#endif
#ifdef APPA
	fprintf(fp, "# %d: appa value, %s\n", col++, appa208_str_unit(mult_unit));
	fprintf(fp, "# %d: appa overload, bool\n", col++);
#endif

	while(get_run(p))
	{
#define ERRL(c, g, s, ...) do{if(c){fprintf(stderr,s,__VA_ARGS__);set_run(p, 0);plot_signal(p);goto g;}}while(0);
		// if (index * step_U > 10)
		// {
		// 	set_run(p, 0);
		// 	plot_signal(p);
		// 	continue;
		// }

#ifdef QJ
		r = qj3003p_set_voltage(qj_fd, index * step_U);
			ERRL(r < 0, worker_close_file, "# E: Unable to set qj voltage (%d)\n", r);
#endif

		// sleep(1);

#ifdef QJ
		r = qj3003p_get_voltage(qj_fd, &pps_U);
			ERRL(r < 0, worker_close_file, "# E: Unable to get qj voltage (%d)\n", r);
#endif

#ifdef QJ
		r = qj3003p_get_current(qj_fd, &pps_I);
			ERRL(r < 0, worker_close_file, "# E: Unable to get qj current (%d)\n", r);
#endif

// printf("1\n");

#ifdef LOMO
		r = lomo_read_value(lomo_fd, &adc);
			ERRL(r < 0, worker_close_file, "# E: Unable to read lyomo adc (%d)\n", r);
#endif

// printf("2\n");

#ifdef APPA
		r = appa208_read_disp(appa_fd, &disp);
			ERRL(r < 0, worker_close_file, "# E: Unable to read appa display (%d)\n", r);
		mult_value = appa208_get_value(&disp.mdata);
		mult_unit = appa208_get_unit(&disp.mdata);
		mult_overload = appa208_get_overload(&disp.mdata);
#endif

// printf("3\n");

		fprintf(fp,
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
			, index
#ifdef QJ
			, pps_U
			, pps_I
#endif
#ifdef LOMO
			, adc
#endif
#ifdef APPA
			, mult_value
			, mult_overload
#endif
		);

		// index++;

		plot_signal(p);
	}

	plot_signal(p);

worker_close_file:

	fclose(fp);

worker_exit_qj:

#ifdef QJ
	r = qj3003p_set_voltage(qj_fd, 0);
		ERRL(r < 0, worker_close_qj, "# E: Unable to set qj voltage (%d)\n", r);
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
		plot_wait(p);

		fprintf(gp, "plot \"%s\" u 3:2 w l\n", p->filename);
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

void plot_wait(params_t *p)
{
	pthread_mutex_lock(&p->plot_mutex);
		pthread_cond_wait(&p->plot_cond, &p->plot_mutex);
	pthread_mutex_unlock(&p->plot_mutex);
}

void plot_signal(params_t *p)
{
	pthread_mutex_lock(&p->plot_mutex);
		pthread_cond_signal(&p->plot_cond);
	pthread_mutex_unlock(&p->plot_mutex);
}