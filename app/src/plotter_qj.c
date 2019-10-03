#include "main.h"

#ifdef QJ
void *plotter_qj(void *arg)
{
	exp_t *p = (exp_t *)arg;

	FILE *gp = NULL;

	const char *gp_cmd = "gnuplot 2>/dev/null";

	gp = popen(gp_cmd, "w");
	if (gp == NULL)
	{
		fprintf(stderr, "# E: Unable to open pipe \"%s\"\n", gp_cmd);
		set_run(p, 0);
		goto plotter_qj_exit;
	}

	setlinebuf(gp);

	fprintf(gp, "set xlabel \"QJ index\"\n");
	fprintf(gp, "set ylabel \"QJ voltage, V\"\n");
	fprintf(gp, "set y2label \"QJ current, A\"\n");
	fprintf(gp, "set xrange [0:]\n");
	fprintf(gp, "set yrange [0:]\n");
	fprintf(gp, "set y2range [0:]\n");
	fprintf(gp, "set y2tics\n");

	while(get_run(p))
	{
		data_qj_written_wait(p);

		fprintf(gp, "plot \"%s\" u 1:2 w l, \"\" u 1:3 w l axes x1y2\n", p->filename_qj);
	}

	pclose(gp);

	plotter_qj_exit:
	return NULL;
}
#endif
