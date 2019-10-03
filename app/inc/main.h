#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/time.h>

#include "lomo.h"
#include "appa208.h"
#include "qj3003p.h"

// #define QJ
// #define LOMO
// #define APPA

#define LOMO_TTY "/dev/ttyUSB2"
#define QJ3003P_TTY "/dev/ttyUSB0"
#define APPA208_TTY "/dev/ttyUSB1"

#define U_STEP 0.1
#define TIME_DELAY_S 5

#define DBG(s)
#define DBGP(s, ...)
// #define DBG(s) do{fprintf(stderr,s);}while(0);
// #define DBGP(s, ...) do{fprintf(stderr,s,__VA_ARGS__);}while(0);
#define ERR(c, g, s, ...) do{if(c){fprintf(stderr,s,__VA_ARGS__);set_run(p, 0);goto g;}}while(0);

// data structs

#ifdef QJ
typedef struct
{
	int index;
	double time;
	double voltage;
	double current;
} data_qj_t;

typedef struct
{
	int pipe;
} exp_qj_t;
#endif

#ifdef LOMO
typedef struct
{
	int index;
	double time;
	double adc;
} data_lomo_t;

typedef struct
{
	int pipe;
} exp_lomo_t;
#endif

#ifdef APPA
typedef struct
{
	int index;
	double time;
	double value;
	unit_t unit;
	int overload;
} data_appa_t;

typedef struct
{
	int pipe;
} exp_appa_t;
#endif

typedef struct
{
	pthread_rwlock_t run_lock;
	int run;
	char filename_log[100];

#ifdef QJ
	char filename_qj[100];
#endif

#ifdef LOMO
	char filename_lomo[100];
#endif

#ifdef APPA
	char filename_appa[100];
#endif

} exp_t;

// treads

void *commander(void *arg);
void *writer(void *arg);

#ifdef QJ
	void *worker_qj(void *arg);
#endif

#ifdef LOMO
	void *worker_lomo(void *arg);
#endif

#ifdef APPA
	void *worker_appa(void *arg);
#endif

// utils

int get_run(exp_t *p);
void set_run(exp_t *p, int run);

double get_time();