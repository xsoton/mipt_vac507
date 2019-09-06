#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "lomo.h"
#include "appa208.h"
#include "qj3003p.h"

#define LOMO_TTY "/dev/ttyUSB1"
#define QJ3003P_TTY "/dev/ttyUSB2"
#define APPA208_TTY "/dev/ttyUSB0"

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

void *worker(void *arg)
{
	params_t *p = (params_t *)arg;
	int r;

	FILE *fp;
	int lomo_fd;
	int appa_fd;
	int qj_fd;

	int index = 0;
	double step_U = 0.1;

	double adc;
	appa208_disp_t disp;
	double mult_value;
	unit_t mult_unit;
	int mult_overload;
	double pps_U;
	double pps_I;

#define DBG(i) fprintf(stderr, "%d\n", i);

#define ERR(c, g, s, ...) do{if(c){fprintf(stderr,s,__VA_ARGS__);set_run(p, 0);goto g;}}while(0);

	r = lomo_open(LOMO_TTY, &lomo_fd);
		ERR(r < 0, worker_exit, "# E: Unable to open lomo (%d)", r);
	r = lomo_init(lomo_fd);
		ERR(r < 0, worker_close_lomo, "# E: Unable to init lomo (%d)", r);

DBG(1);

	r = appa208_open(APPA208_TTY, &appa_fd);
		ERR(r < 0, worker_close_lomo, "# E: Unable to open appa (%d)", r);
	r = appa208_read_disp(appa_fd, &disp);
		ERR(r < 0, worker_close_appa, "# E: Unable to read appa display (%d)", r);
	mult_unit = appa208_get_unit(&disp.mdata);

DBG(1);

	r = qj3003p_open(QJ3003P_TTY, &qj_fd);
		ERR(r < 0, worker_close_appa, "# E: Unable to open qj (%d)", r);
	r = qj3003p_set_output(qj_fd, 0);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj output (%d)", r);
	r = qj3003p_set_voltage(qj_fd, 0.0);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj voltage (%d)", r);
	r = qj3003p_set_current(qj_fd, 3.0);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj current (%d)", r);
	r = qj3003p_set_output(qj_fd, 1);
		ERR(r < 0, worker_exit_qj, "# E: Unable to set qj output (%d)", r);

DBG(3);

	fp = fopen(p->filename, "w+");
		ERR(fp == NULL, worker_exit_qj, "# E: Unable to open file \"%s\"\n", p->filename);

	setlinebuf(fp);

	fprintf(fp, "# 1: index\n");
	fprintf(fp, "# 2: pps U, V\n");
	fprintf(fp, "# 3: pps I, A\n");
	fprintf(fp, "# 4: lomo adc, a.u.\n");
	fprintf(fp, "# 5: appa value, %s\n", appa208_str_unit(mult_unit));
	fprintf(fp, "# 6: appa overload, bool\n");

	while(get_run(p))
	{
DBG(4);
#define ERRL(c, g, s, ...) do{if(c){fprintf(stderr,s,__VA_ARGS__);set_run(p, 0);plot_signal(p);goto g;}}while(0);
		r = qj3003p_set_voltage(qj_fd, index * step_U);
			ERRL(r < 0, worker_close_file, "# E: Unable to set qj voltage (%d)", r);

DBG(5);

		sleep(1);

DBG(6);

		r = qj3003p_get_voltage(qj_fd, &pps_U);
			ERRL(r < 0, worker_close_file, "# E: Unable to get qj voltage (%d)", r);
		r = qj3003p_get_current(qj_fd, &pps_I);
			ERRL(r < 0, worker_close_file, "# E: Unable to get qj current (%d)", r);

DBG(7);

		r = lomo_read_value(lomo_fd, &adc);
			ERRL(r < 0, worker_close_file, "# E: Unable to read lomo adc (%d)", r);

DBG(8);

		r = appa208_read_disp(appa_fd, &disp);
			ERRL(r < 0, worker_close_file, "# E: Unable to read appa display (%d)", r);
		mult_value = appa208_get_value(&disp.mdata);
		mult_unit = appa208_get_unit(&disp.mdata);
		mult_overload = appa208_get_overload(&disp.mdata);

DBG(9);

		fprintf(fp, "%d\t%le\t%le\t%le\t%le\t%d\n",
			index, pps_U, pps_I, adc, mult_value, mult_overload);

		index++;

		plot_signal(p);
	}

worker_close_file:
	fclose(fp);

worker_exit_qj:
	r = qj3003p_set_output(qj_fd, 0);
		ERR(r < 0, worker_close_qj, "# E: Unable to set qj voltage (%d)", r);
worker_close_qj:
	r = qj3003p_close(qj_fd);
		ERR(r < 0, worker_close_appa, "# E: Unable to close qj (%d)", r);

worker_close_appa:
	r = appa208_close(appa_fd);
		ERR(r < 0, worker_close_lomo, "# E: Unable to close appa (%d)", r);

worker_close_lomo:
	r = lomo_close(lomo_fd);
		ERR(r < 0, worker_exit, "# E: Unable to close lomo (%d)", r);

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
	fprintf(gp, "set ylabel \"ADC signal, 24 bit\"\n");

	while(get_run(p))
	{
		plot_wait(p);

		fprintf(gp, "plot \"%s\" u 2:3 w l\n", p->filename);
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