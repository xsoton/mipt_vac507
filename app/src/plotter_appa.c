#include "main.h"

#ifdef APPA
void *plotter_appa(void *arg)
{
	exp_t *p = (exp_t *)arg;

	FILE *gp = NULL;

	const char *gp_cmd = "gnuplot 2>/dev/null";

	gp = popen(gp_cmd, "w");
	if (gp == NULL)
	{
		fprintf(stderr, "# E: Unable to open pipe \"%s\"\n", gp_cmd);
		set_run(p, 0);
		goto plotter_appa_exit;
	}

	setlinebuf(gp);

	fprintf(gp, "set xlabel \"APPA index\"\n");
	fprintf(gp, "set ylabel \"APPA voltage, V\"\n");
	fprintf(gp, "set y2label \"APPA overload\"\n");
	fprintf(gp, "set xrange [0:]\n");
	fprintf(gp, "set yrange [0:]\n");
	fprintf(gp, "set y2range [0:]\n");
	fprintf(gp, "set y2tics\n");

	while(get_run(p))
	{
		data_appa_written_wait(p);

		fprintf(gp, "plot \"%s\" u 1:2 w l, \"\" u 1:3 w l axes x1y2\n", p->filename_appa);
	}

	pclose(gp);

	plotter_appa_exit:
	return NULL;
}
#endif
