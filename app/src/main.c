#include <stdio.h>

#include "qj3003p.h"

#define QJ3003P_TTY "/dev/ttyUSB2"

int main(int argc, char const *argv[])
{
	(void)argc;
	(void)argv;

	int ret = 0;
	int r;

	int qj3003p_fd;

	r = qj3003p_open(QJ3003P_TTY, &qj3003p_fd);
	if (r < 0)
	{
		fprintf(stderr, "# E: unable to open " QJ3003P_TTY " as QJ3003P device (%d)\n", r);
		ret = -1;
		goto main_exit;
	}

	{
		#define MAX_STR_LENGTH 1024
		char str[MAX_STR_LENGTH] = {0};

		r = qj3003p_get_idn(qj3003p_fd, str, MAX_STR_LENGTH);
		if (r < 0)
		{
			fprintf(stderr, "# E: unable to get idn of QJ3003P device (%d)\n", r);
			ret = -1;
			goto main_close;
		}

		fprintf(stdout, "idn = \"%s\"\n", str);
	}

	main_close:
	r = qj3003p_close(qj3003p_fd);
	if (r < 0)
	{
		ret = -1;
		goto main_exit;
	}

	main_exit:
	return ret;
}