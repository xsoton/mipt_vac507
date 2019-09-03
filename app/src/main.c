#include <stdio.h>

#include "appa208.h"

#define LOMO_TTY "/dev/ttyUSB0"
#define QJ3003P_TTY "/dev/ttyUSB1"
#define APPA208_TTY "/dev/ttyUSB2"

int main(int argc, char const *argv[])
{
	(void)argc;
	(void)argv;

	int ret = 0;
	int r;

	int appa208_fd;
	appa208_info_t info;
	appa208_disp_t disp;
	double value;
	unit_t unit;
	int overload;

	r = appa208_open(APPA208_TTY, &appa208_fd);
	if (r < 0)
	{
		fprintf(stderr, "# E: unable to open APPA208 (%d)\n", r);
		ret = -1;
		goto main_exit;
	}

	r = appa208_read_info(appa208_fd, &info);
	if (r < 0)
	{
		fprintf(stderr, "# E: unable to read info from APPA208 (%d)\n", r);
		ret = -2;
		goto main_close;
	}

	printf("info:\n");
	printf("model_name = %.32s\n", info.model_name);
	printf("serial_name = %.16s\n", info.serial_name);
	uint16_t mid = *(uint16_t *)info.model_id;
	uint16_t fwv = *(uint16_t *)info.fw_version;
	printf("model_id = %u\n", mid);
	printf("fw_version = %u\n", fwv);

	r = appa208_read_disp(appa208_fd, &disp);
	if (r < 0)
	{
		fprintf(stderr, "# E: unable to read disp from APPA208 (%d)\n", r);
		ret = -3;
		goto main_close;
	}

	value = appa208_get_value(&disp.mdata);
	unit = appa208_get_unit(&disp.mdata);
	overload = appa208_get_overload(&disp.mdata);

	printf("value = %le (%s), overload = %d\n", value, appa208_units[unit], overload);


main_close:
	r = appa208_close(appa208_fd);
	if (r < 0)
	{
		fprintf(stderr, "# E: unable to close APPA208 (%d)\n", r);
		ret = -4;
		goto main_exit;
	}

main_exit:
	return ret;
}