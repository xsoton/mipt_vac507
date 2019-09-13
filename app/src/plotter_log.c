#include "main.h"

void *plotter_log(void *arg)
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

	fprintf(gp, "set xlabel \"QJ voltage, V\"\n");
	fprintf(gp, "set ylabel \"LOMO adc signal, a.u. [0-1]\"\n");
	fprintf(gp, "set xrange [0:]\n");
	fprintf(gp, "set yrange [0:]\n");

	while(get_run(p))
	{
		data_log_written_wait(p);

		fprintf(gp, "plot \"%s\" u 3:6 w l\n", p->filename_log);
	}

	pclose(gp);

plotter_exit:

	return NULL;
}
