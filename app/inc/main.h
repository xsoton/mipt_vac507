#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include "lomo.h"
#include "appa208.h"
#include "qj3003p.h"

// #define QJ
#define LOMO
// #define APPA

#define LOMO_TTY "/dev/ttyUSB2"
#define QJ3003P_TTY "/dev/ttyUSB0"
#define APPA208_TTY "/dev/ttyUSB1"

#define U_STEP 0.1
#define TIME_DELAY_S 10

#define DBG(s)
#define DBGP(s, ...)
// #define DBG(s) do{fprintf(stderr,s);}while(0);
// #define DBGP(s, ...) do{fprintf(stderr,s,__VA_ARGS__);}while(0);
#define ERR(c, g, s, ...) do{if(c){fprintf(stderr,s,__VA_ARGS__);set_run(p, 0);goto g;}}while(0);

typedef struct
{
	pthread_rwlock_t run_lock;
	int run;
	int eq_ready;
	pthread_mutex_t data_ready_mutex;
	pthread_cond_t data_ready_cond;
	pthread_mutex_t data_log_written_mutex;
	pthread_cond_t data_log_written_cond;
	char filename_log[100];
	pthread_rwlock_t data_lock;
#ifdef QJ
	pthread_mutex_t data_qj_written_mutex;
	pthread_cond_t data_qj_written_cond;
	int index_qj;
	int ready_qj;
	double pps_U;
	double pps_I;
	char filename_qj[100];
#endif
#ifdef LOMO
	pthread_mutex_t data_lomo_written_mutex;
	pthread_cond_t data_lomo_written_cond;
	int index_lomo;
	int ready_lomo;
	double adc;
	char filename_lomo[100];
#endif
#ifdef APPA
	pthread_mutex_t data_appa_written_mutex;
	pthread_cond_t data_appa_written_cond;
	int index_appa;
	int ready_appa;
	double mult_value;
	unit_t mult_unit;
	int mult_overload;
	char filename_appa[100];
#endif
} params_t;

void *commander(void *arg);

#ifdef QJ
	void *worker_qj(void *arg);
	void *plotter_qj(void *arg);
#endif

#ifdef LOMO
	void *worker_lomo(void *arg);
	void *plotter_lomo(void *arg);
#endif

#ifdef APPA
	void *worker_appa(void *arg);
	void *plotter_appa(void *arg);
#endif

void *writer_log(void *arg);
void *plotter_log(void *arg);

int get_run(params_t *p);
void set_run(params_t *p, int run);

void data_ready_wait(params_t *p);
void data_ready_signal(params_t *p);

void data_log_written_wait(params_t *p);
void data_log_written_signal(params_t *p);
void data_log_written_broadcast(params_t *p);

#ifdef QJ
	void data_qj_written_wait(params_t *p);
	void data_qj_written_signal(params_t *p);
	void data_qj_written_broadcast(params_t *p);
#endif

#ifdef LOMO
	void data_lomo_written_wait(params_t *p);
	void data_lomo_written_signal(params_t *p);
	void data_lomo_written_broadcast(params_t *p);
#endif

#ifdef APPA
	void data_appa_written_wait(params_t *p);
	void data_appa_written_signal(params_t *p);
	void data_appa_written_broadcast(params_t *p);
#endif
