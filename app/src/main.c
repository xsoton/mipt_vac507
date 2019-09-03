#include <stdio.h>

#include "lomo.h"

#define LOMO_TTY "/dev/ttyUSB1"
#define QJ3003P_TTY "/dev/ttyUSB2"
#define APPA208_TTY "/dev/ttyUSB0"

int main(int argc, char const *argv[])
{
	(void)argc;
	(void)argv;

	int ret = 0;
	int r;

	int lomo_fd;
	double value;

	r = lomo_open(LOMO_TTY, &lomo_fd);
	if (r < 0)
	{
		ret = -1;
		fprintf(stderr, "# E: unable to open LOMO (%d)\n", r);
		goto main_exit;
	}

	r = lomo_read_init(lomo_fd);
	if (r < 0)
	{
		ret = -1;
		fprintf(stderr, "# E: unable to init LOMO (%d)\n", r);
		goto main_close;
	}

	r = lomo_read_value(lomo_fd, &value);
	if (r < 0)
	{
		ret = -1;
		fprintf(stderr, "# E: unable to read value from LOMO (%d)\n", r);
		goto main_close;
	}

	printf("value = %le V\n", value);

main_close:

	r = lomo_close(lomo_fd);
	if (r < 0)
	{
		ret = -1;
		fprintf(stderr, "# E: unable to close LOMO (%d)\n", r);
		goto main_exit;
	}

main_exit:

	return ret;
}