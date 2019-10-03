#include "main.h"

int main(int argc, char const *argv[])
{
	int ret = 0;

	exp_t exp = {0};

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

	pthread_t t_writer;
	pthread_t t_plotter_log;

	if (argc < 2)
	{
		fprintf(stderr, "# E: Usage: vac <experiment_name>\n");
		ret = -1;
		goto main_exit;
	}

	setlinebuf(stdout);
	setlinebuf(stderr);

	pthread_rwlock_init(&exp.run_lock, NULL);
	pthread_mutex_init(&exp.data_ready_mutex, NULL);
	pthread_cond_init(&exp.data_ready_cond, NULL);
	pthread_mutex_init(&exp.data_log_written_mutex, NULL);
	pthread_cond_init(&exp.data_log_written_cond, NULL);
	snprintf(exp.filename_log, 100, "%s_log.dat", argv[1]);
	pthread_rwlock_init(&exp.data_lock, NULL);
	exp.run = 1;

	#ifdef QJ
		pthread_mutex_init(&exp.data_qj_written_mutex, NULL);
		pthread_cond_init(&exp.data_qj_written_cond, NULL);
		snprintf(exp.filename_qj, 100, "%s_qj.dat", argv[1]);
	#endif

	#ifdef LOMO
		pthread_mutex_init(&exp.data_lomo_written_mutex, NULL);
		pthread_cond_init(&exp.data_lomo_written_cond, NULL);
		snprintf(exp.filename_lomo, 100, "%s_lomo.dat", argv[1]);
	#endif

	#ifdef APPA
		pthread_mutex_init(&exp.data_appa_written_mutex, NULL);
		pthread_cond_init(&exp.data_appa_written_cond, NULL);
		snprintf(exp.filename_appa, 100, "%s_appa.dat", argv[1]);
	#endif


	pthread_create(&t_commander, NULL, commander, &exp);

	#ifdef QJ
		pthread_create(&t_worker_qj, NULL, worker_qj, &exp);
		pthread_create(&t_plotter_qj, NULL, plotter_qj, &exp);
	#endif

	#ifdef LOMO
		pthread_create(&t_worker_lomo, NULL, worker_lomo, &exp);
		pthread_create(&t_plotter_lomo, NULL, plotter_lomo, &exp);
	#endif

	#ifdef APPA
		pthread_create(&t_worker_appa, NULL, worker_appa, &exp);
		pthread_create(&t_plotter_appa, NULL, plotter_appa, &exp);
	#endif

	pthread_create(&t_writer, NULL, writer, &exp);
	pthread_create(&t_plotter_log, NULL, plotter_log, &exp);

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
	pthread_join(t_writer, NULL);
	pthread_join(t_plotter_log, NULL);

	pthread_cancel(t_commander);
	pthread_join(t_commander, NULL);

main_exit:

	return ret;
}






















int main(int argc, char const *argv[])
{
	int ret = 0;

	exp_t exp = {0};
	pthread_t t_commander;
	pthread_t t_writer;
	int pfd[2];

	#ifdef QJ
		exp_qj_t exp_qj = {0};
		pthread_t t_worker_qj;
	#endif

	#ifdef LOMO
		exp_lomo_t exp_lomo = {0};
		pthread_t t_worker_lomo;
	#endif

	#ifdef APPA
		exp_appa_t exp_appa = {0};
		pthread_t t_worker_appa;
	#endif

	if (argc < 2)
	{
		fprintf(stderr, "# E: Usage: vac <experiment_name>\n");
		ret = -1;
		goto main_exit;
	}

	setlinebuf(stdout);
	setlinebuf(stderr);

	pipe(&pfd);

	pthread_rwlock_init(&exp.run_lock, NULL);
	snprintf(exp.filename_log, 100, "%s_log.dat", argv[1]);
	exp.run = 1;

	pthread_create(&t_commander, NULL, commander, NULL);
	pthread_create(&t_writer, NULL, writer, &exp);

	#ifdef QJ
		snprintf(exp.filename_qj, 100, "%s_qj.dat", argv[1]);
		pthread_create(&t_worker_qj, NULL, worker_qj, &exp_qj);
	#endif

	#ifdef LOMO
		snprintf(exp.filename_lomo, 100, "%s_lomo.dat", argv[1]);
		pthread_create(&t_worker_lomo, NULL, worker_lomo, &exp_lomo);
	#endif

	#ifdef APPA
		snprintf(exp.filename_appa, 100, "%s_appa.dat", argv[1]);
		pthread_create(&t_worker_appa, NULL, worker_appa, &exp_appa);
	#endif


	#ifdef QJ
		pthread_join(t_worker_qj, NULL);
	#endif

	#ifdef LOMO
		pthread_join(t_worker_lomo, NULL);
	#endif

	#ifdef APPA
		pthread_join(t_worker_appa, NULL);
	#endif

	pthread_join(t_writer, NULL);

	pthread_cancel(t_commander);
	pthread_join(t_commander, NULL);

main_exit:

	return ret;
}
