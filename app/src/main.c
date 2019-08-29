#include <stdio.h>

#include "qj3003p.h"

#define QJ3003P_TTY "/dev/ttyUSB2"

int main(int argc, char const *argv[])
{
	(void)argc;
	(void)argv;

	int qj3003p_fd;

	qj3003p_open(QJ3003P_TTY, &qj3003p_fd);

	#define MAX_STR_LENGTH 1024
	char str[MAX_STR_LENGTH] = {0};

	double current;
	double voltage;

	int output;

	qj3003p_get_idn(qj3003p_fd, str, MAX_STR_LENGTH);
	printf("idn = \"%s\"\n", str);

	qj3003p_get_status(qj3003p_fd, &output);
	printf("output = %d\n", output);

	qj3003p_set_output(qj3003p_fd, 0);

	current = 1.234;
	qj3003p_set_current(qj3003p_fd, current);
	printf("iset = %lf\n", current);

	voltage = 5.678;
	qj3003p_set_voltage(qj3003p_fd, voltage);
	printf("vset = %lf\n", voltage);

	qj3003p_set_output(qj3003p_fd, 1);

	qj3003p_get_status(qj3003p_fd, &output);
	printf("output = %d\n", output);

	qj3003p_get_current(qj3003p_fd, &current);
	printf("iout = %lf\n", current);

	qj3003p_get_voltage(qj3003p_fd, &voltage);
	printf("vout = %lf\n", voltage);

	qj3003p_set_output(qj3003p_fd, 0);

	qj3003p_get_status(qj3003p_fd, &output);
	printf("output = %d\n", output);

	qj3003p_close(qj3003p_fd);

	return 0;
}