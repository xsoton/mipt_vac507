#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

// === hardware includes

#include "appa208.h"
#include "qj3003p.h"

// === config
// TODO: make a config file

#define APPA208_TTY "/dev/ttyUSB1"
#define QJ3003P_TTY "/dev/ttyUSB0"
#define VOLTAGE_STEP 0.5

// === threads

void *commander(void *);
void *worker(void *);

// === utils

int get_run();
void set_run(int run_new);
double get_time();

// === global variables

char dir_str[100];

pthread_rwlock_t run_lock;
int run;
char filename_vac[100];
time_t start_time;
struct tm start_time_struct;

// === program entry point

int main(int argc, char const *argv[])
{
	int ret = 0;

	pthread_t t_commander;
	pthread_t t_worker;

	int status;

	// === check input parameters

	if (argc < 2)
	{
		fprintf(stderr, "# E: Usage: vac <experiment_name>\n");
		ret = -1;
		goto main_exit;
	}

	// === get start time of experiment

	start_time = time(NULL);
	localtime_r(&start_time, &start_time_struct);

	// === we need actual information w/o buffering

	setlinebuf(stdout);
	setlinebuf(stderr);

	// === initialize run state variable

	pthread_rwlock_init(&run_lock, NULL);
	run = 1;

	// === create dirictory in "20191012_153504_<experiment_name>" format

	snprintf(dir_str, 100, "%04d%02d%02d_%02d%02d%02d_%s",
		start_time_struct.tm_year + 1900,
		start_time_struct.tm_mon + 1,
		start_time_struct.tm_mday,
		start_time_struct.tm_hour,
		start_time_struct.tm_min,
		start_time_struct.tm_sec,
		argv[1]
	);
	status = mkdir(dir_str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (status == -1)
	{
		fprintf(stderr, "# E: unable to create experiment directory (%s)\n", strerror(errno));
		ret = -2;
		goto main_exit;
	}

	// === create file names

	snprintf(filename_vac, 100, "%s/vac.dat", dir_str);

	// printf("filename_vac \"%s\"\n", filename_vac);

	// === now start threads

	pthread_create(&t_commander, NULL, commander, NULL);
	pthread_create(&t_worker, NULL, worker, NULL);

	// === and wait ...

	pthread_join(t_worker, NULL);

	// === cancel commander thread becouse we don't need it anymore
	// === and wait for cancelation finish

	pthread_cancel(t_commander);
	pthread_join(t_commander, NULL);

	main_exit:
	return ret;
}

// === commander function

void *commander(void *arg)
{
	(void) arg;

	char str[100];
	char *s;
	int ccount;

	while(get_run())
	{
		printf("> ");

		s = fgets(str, 100, stdin);
		if (s == NULL)
		{
			fprintf(stderr, "# E: Exit\n");
			set_run(0);
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
				set_run(0);
				break;
			default:
				ccount = strlen(str)-1;
				fprintf(stderr, "# E: Unknown commang (%.*s)\n", ccount, str);
				break;
		}
	}

	return NULL;
}

// === worker function

void *worker(void *arg)
{
	(void) arg;

	int r;

	int    ps_fd;
	int    ps_index;
	double ps_time;
	double ps_voltage;
	double ps_current;

	int         vm_fd;
	double      vm_value;

	FILE  *vac_fp;

	FILE  *gp;

	char   buf[100];

	// === first we connect to instruments

	r = qj3003p_open(QJ3003P_TTY, &ps_fd);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to open ps (%d)\n", r);
		goto worker_exit;
	}

	r = appa208_open(APPA208_TTY, &vm_fd);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to open voltmeter (%d)\n", r);
		set_run(0);
		goto worker_ps_close;
	}

	// === init ps

	r = qj3003p_set_output(ps_fd, 0);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to set ps output (%d)\n", r);
		goto worker_adc_close;
	}

	qj3003p_delay();

	r = qj3003p_set_voltage(ps_fd, 0.0);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to set ps voltage (%d)\n", r);
		goto worker_adc_close;
	}

	qj3003p_delay();

	r = qj3003p_set_current(ps_fd, 0.5);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to set ps current (%d)\n", r);
		goto worker_adc_close;
	}

	qj3003p_delay();

	r = qj3003p_set_output(ps_fd, 1);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to set ps output (%d)\n", r);
		goto worker_adc_close;
	}

	qj3003p_delay();

	// === create vac file

	vac_fp = fopen(filename_vac, "w+");
	if(vac_fp == NULL)
	{
		fprintf(stderr, "# E: Unable to open file \"%s\" (%s)\n", filename_vac, strerror(ferror(vac_fp)));
		goto worker_ps_deinit;
	}

	setlinebuf(vac_fp);

	// === write vac header

	r = fprintf(vac_fp,
		"# 1: index\n"
		"# 2: time, s\n"
		"# 3: ps voltage, V\n"
		"# 4: ps current, A\n"
		"# 5: vm value, V\n");
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to print to file \"%s\" (%s)\n", filename_vac, strerror(r));
		goto worker_adc_fp_close;
	}

	// === open gnuplot

	snprintf(buf, 100, "gnuplot > %s/gnuplot.log 2>&1", dir_str);
	gp = popen(buf, "w");
	if (gp == NULL)
	{
		fprintf(stderr, "# E: unable to open gnuplot pipe (%s)\n", strerror(errno));
		goto worker_adc_fp_close;
	}

	setlinebuf(gp);

	// === prepare gnuplot

	r = fprintf(gp,
		"set xrange [0:]\n"
		"set yrange [0:]\n"
		"set xlabel \"PS voltage, V\"\n"
		"set ylabel \"VM signal, V\"\n"
	);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to print to gp (%s)\n", strerror(r));
		goto worker_gp_close;
	}

	// === let the action begins!

	ps_index = 0;

	while(get_run())
	{
		appa208_disp_t disp;

		ps_time = get_time();
		if (ps_time < 0)
		{
			fprintf(stderr, "# E: Unable to get time\n");
			set_run(0);
			continue;
		}

		if ((ps_index + 1) * VOLTAGE_STEP > 10)
		{
			set_run(0);
			continue;
		}

		r = qj3003p_set_voltage(ps_fd, ps_index * VOLTAGE_STEP);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to set qj voltage (%d)\n", r);
			set_run(0);
			continue;
		}

		qj3003p_delay();

		sleep(1);

		r = appa208_read_disp(vm_fd, &disp);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to read appa (%d)\n", r);
			set_run(0);
			continue;
		}

		vm_value = appa208_get_value(&disp.mdata);

		r = qj3003p_get_voltage(ps_fd, &ps_voltage);
		if(r < 0)
		{
			fprintf(stderr,"# E: Unable to get ps voltage (%d)\n", r);
			set_run(0);
			continue;
		}

		r = qj3003p_get_current(ps_fd, &ps_current);
		if(r < 0)
		{
			fprintf(stderr,"# E: Unable to get ps current (%d)\n", r);
			set_run(0);
			continue;
		}

		r = fprintf(vac_fp, "%d\t%le\t%le\t%le\t%le\n", ps_index, ps_time, ps_voltage, ps_current, vm_value);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to print to file \"%s\" (%s)\n", filename_vac, strerror(r));
			set_run(0);
			continue;
		}

		r = fprintf(gp,
			"plot \"%s\" u 3:5 w l lw 1 notitle\n",
			filename_vac
		);
		if(r < 0)
		{
			fprintf(stderr, "# E: Unable to print to gp (%s)\n", strerror(r));
			set_run(0);
			continue;
		}

		ps_index++;

		worker_while_continue:
		continue;
	}

	r = qj3003p_set_voltage(ps_fd, 0.0);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to set ps voltage (%d)\n", r);
	}

	qj3003p_delay();

	worker_gp_close:

	r = pclose(gp);
	if (r == -1)
	{
		fprintf(stderr, "# E: Unable to close gnuplot pipe (%s)\n", strerror(errno));
	}

	worker_adc_fp_close:

	worker_vac_fp_close:

	r = fclose(vac_fp);
	if (r == EOF)
	{
		fprintf(stderr, "# E: Unable to close file \"%s\" (%s)\n", filename_vac, strerror(errno));
	}

	worker_ps_deinit:

	r = qj3003p_set_voltage(ps_fd, 0);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to set ps voltage (%d)\n", r);
	}

	qj3003p_delay();

	r = qj3003p_set_output(ps_fd, 0);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to set ps output (%d)\n", r);
	}

	qj3003p_delay();

	worker_adc_close:

	r = appa208_close(vm_fd);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to close appa (%d)\n", r);
	}

	worker_ps_close:

	r = qj3003p_close(ps_fd);
	if(r < 0)
	{
		fprintf(stderr, "# E: Unable to close adc (%d)\n", r);
	}

	worker_exit:

	return NULL;
}

// === utils

int get_run()
{
	int run_local;
	pthread_rwlock_rdlock(&run_lock);
		run_local = run;
	pthread_rwlock_unlock(&run_lock);
	return run_local;
}

void set_run(int run_new)
{
	pthread_rwlock_wrlock(&run_lock);
		run = run_new;
	pthread_rwlock_unlock(&run_lock);
}

double get_time()
{
	static int first = 1;
	static struct timeval t_first = {0};
	struct timeval t = {0};
	double ret;
	int r;

	if (first == 1)
	{
		r = gettimeofday(&t_first, NULL);
		if (r == -1)
		{
			fprintf(stderr, "# E: unable to get time (%s)\n", strerror(errno));
			ret = -1;
		}
		else
		{
			ret = 0.0;
			first = 0;
		}
	}
	else
	{
		r = gettimeofday(&t, NULL);
		if (r == -1)
		{
			fprintf(stderr, "# E: unable to get time (%s)\n", strerror(errno));
			ret = -2;
		}
		else
		{
			ret = (t.tv_sec - t_first.tv_sec) * 1e6 + (t.tv_usec - t_first.tv_usec);
			ret /= 1e6;
		}
	}

	return ret;
}