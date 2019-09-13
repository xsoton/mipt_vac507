#include "main.h"

int main(int argc, char const *argv[])
{
	int ret = 0;

	params_t params = {0};

	pthread_t t_commander;

	#ifdef QJ
		pthread_t t_worker_qj;
		pthread_t t_plotter_qj;
	#endif

	#ifdef LOMO
		pthread_t t_worker_lomo;
		pthread_t t_plotter_lomo;
	#endif

	#ifdef APPA
		pthread_t t_worker_appa;
		pthread_t t_plotter_appa;
	#endif

	pthread_t t_writer_log;
	pthread_t t_plotter_log;

	if (argc < 2)
	{
		fprintf(stderr, "# E: Usage: vac <experiment_name>\n");
		ret = -1;
		goto main_exit;
	}

	setlinebuf(stdout);
	setlinebuf(stderr);

	pthread_rwlock_init(&params.run_lock, NULL);
	pthread_mutex_init(&params.data_ready_mutex, NULL);
	pthread_cond_init(&params.data_ready_cond, NULL);
	pthread_mutex_init(&params.data_log_written_mutex, NULL);
	pthread_cond_init(&params.data_log_written_cond, NULL);
	snprintf(params.filename_log, 100, "%s_log.dat", argv[1]);
	pthread_rwlock_init(&params.data_lock, NULL);
	params.run = 1;

	#ifdef QJ
		pthread_mutex_init(&params.data_qj_written_mutex, NULL);
		pthread_cond_init(&params.data_qj_written_cond, NULL);
		snprintf(params.filename_qj, 100, "%s_qj.dat", argv[1]);
	#endif

	#ifdef LOMO
		pthread_mutex_init(&params.data_lomo_written_mutex, NULL);
		pthread_cond_init(&params.data_lomo_written_cond, NULL);
		snprintf(params.filename_lomo, 100, "%s_lomo.dat", argv[1]);
	#endif

	#ifdef APPA
		pthread_mutex_init(&params.data_appa_written_mutex, NULL);
		pthread_cond_init(&params.data_appa_written_cond, NULL);
		snprintf(params.filename_appa, 100, "%s_appa.dat", argv[1]);
	#endif


	pthread_create(&t_commander, NULL, commander, &params);

	#ifdef QJ
		pthread_create(&t_worker_qj, NULL, worker_qj, &params);
		pthread_create(&t_plotter_qj, NULL, plotter_qj, &params);
	#endif

	#ifdef LOMO
		pthread_create(&t_worker_lomo, NULL, worker_lomo, &params);
		pthread_create(&t_plotter_lomo, NULL, plotter_lomo, &params);
	#endif

	#ifdef APPA
		pthread_create(&t_worker_appa, NULL, worker_appa, &params);
		pthread_create(&t_plotter_appa, NULL, plotter_appa, &params);
	#endif

	pthread_create(&t_writer_log, NULL, writer_log, &params);
	pthread_create(&t_plotter_log, NULL, plotter_log, &params);

	#ifdef QJ
		pthread_join(t_worker_qj, NULL);
		pthread_join(t_plotter_qj, NULL);
	#endif

	#ifdef LOMO
		pthread_join(t_worker_lomo, NULL);
		pthread_join(t_plotter_lomo, NULL);
	#endif

	#ifdef APPA
		pthread_join(t_worker_appa, NULL);
		pthread_join(t_plotter_appa, NULL);
	#endif
	pthread_join(t_writer_log, NULL);
	pthread_join(t_plotter_log, NULL);

	pthread_cancel(t_commander);
	pthread_join(t_commander, NULL);

main_exit:

	return ret;
}
