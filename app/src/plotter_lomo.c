#include "main.h"

#ifdef LOMO
void *plotter_lomo(void *arg)
{
	exp_t *p = (exp_t *)arg;

	FILE *gp = NULL;

	const char *gp_cmd = "gnuplot 2>/dev/null";

	gp = popen(gp_cmd, "w");
	if (gp == NULL)
	{
		fprintf(stderr, "# E: Unable to open pipe \"%s\"\n", gp_cmd);
		set_run(p, 0);
		goto plotter_lomo_exit;
	}

	setlinebuf(gp);

	fprintf(gp, "set xlabel \"LOMO index\"\n");
	fprintf(gp, "set ylabel \"LOMO adc signal, a.u. [0-1]\"\n");
	fprintf(gp, "set xrange [0:]\n");
	fprintf(gp, "set yrange [0:]\n");

	while(get_run(p))
	{
		data_lomo_written_wait(p);

		fprintf(gp, "plot \"%s\" u 1:2 w l\n", p->filename_lomo);
	}

	pclose(gp);

	plotter_lomo_exit:
	return NULL;
}
#endif
