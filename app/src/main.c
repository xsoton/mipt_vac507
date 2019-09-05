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
	pthread_rwlock_t data_lock;
	int index;
	double V;
	uint32_t adc;
	char filename[100];
	int plot_ready;
} params_t;

void *commander(void *arg);
void *worker(void *arg);
void *plotter(void *arg);

int main(int argc, char const *argv[])
{
	int ret = 0;
	// int r;

	params_t params = {PTHREAD_RWLOCK_INITIALIZER, 1, PTHREAD_RWLOCK_INITIALIZER, 0, 0, 0, {0}, 0};

	pthread_t t_commander;
	pthread_t t_worker;
	pthread_t t_plotter;

	if (argc < 2)
	{
		fprintf(stderr, "# E: Usage: vac <filename.dat>\n");
		ret = -1;
		goto main_exit;
	}

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
	char str[100];
	char *s;

	params_t *params = (params_t *)arg;
	int run_local;

	while(1)
	{
		pthread_rwlock_rdlock(&params->run_lock);
			run_local = params->run;
		pthread_rwlock_unlock(&params->run_lock);

		if(run_local == 0)
		{
			break;
		}

		printf("> ");

		s = fgets(str, 100, stdin);
		if (s == NULL)
		{
			fprintf(stderr, "# E: Exit\n");
			pthread_rwlock_wrlock(&params->run_lock);
				params->run = 0;
			pthread_rwlock_unlock(&params->run_lock);
			break;
		}

		switch(str[0])
		{
			case 'h':
				printf(
					"Help:\n"
					"\th -- this help;\n"
					"\tq -- exit the program;\n");
				break;
			case 'q':
				pthread_rwlock_wrlock(&params->run_lock);
					params->run = 0;
				pthread_rwlock_unlock(&params->run_lock);
				break;
			default:
				fprintf(stderr, "# E: Unknown commang (%.*s)\n", strlen(str)-1, str);
				break;
		}
	}

	return NULL;
}

void *worker(void *arg)
{
	params_t *params = (params_t *)arg;



	return NULL;
}

void *plotter(void *arg)
{
	params_t *params = (params_t *)arg;

	FILE *gp = NULL;
	int run_local;
	int plot_ready_local;

	gp = popen("gnuplot 2>/dev/null", "w");
	if (gp == NULL)
	{
		goto plotter_exit;
	}
	setlinebuf(gp);

	fprintf(gp, "set xlabel \"Voltage, V\"\n");
	fprintf(gp, "set ylabel \"ADC signal, 24 bit\"\n");

	while(1)
	{
		pthread_rwlock_rdlock(&params->run_lock);
			run_local = params->run;
			plot_ready_local = params->plot_ready;
		pthread_rwlock_unlock(&params->run_lock);

		if(run_local == 0)
		{
			break;
		}

		if (plot_ready_local)
		{
			fprintf(gp, "plot \"%s\" u 2:3 w l\n", params->filename);
			plot_ready_local = 0;
			pthread_rwlock_rdlock(&params->run_lock);
				params->plot_ready = plot_ready_local;
			pthread_rwlock_unlock(&params->run_lock);
		}
	}

plotter_pclose:

	pclose(gp);

plotter_exit:

	return NULL;
}
