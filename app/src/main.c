#include <stdio.h>

#include "appa208.h"

#define QJ3003P_TTY "/dev/ttyUSB2"
#define APPA208_TTY "/dev/ttyUSB0"

int main(int argc, char const *argv[])
{
	(void)argc;
	(void)argv;

	int appa208_fd;
	char model[32] = {0};
	char serial[16] = {0};
	uint16_t model_id = 0;
	uint16_t fw_version = 0;

	appa208_open(APPA208_TTY, &appa208_fd);

	appa208_read_info(appa208_fd, model, serial, &model_id, &fw_version);

	printf("model = \"%.32s\"\n", model);
	printf("serial = \"%.16s\"\n", serial);
	printf("model_id = 0x%x\n", model_id);
	printf("fw_version = 0x%x\n", fw_version);

	appa208_close(appa208_fd);

	return 0;
}