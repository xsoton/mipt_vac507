#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "appa208.h"

//      CMD           CODE Data length: PC->Meter Meter->PC
#define CMD_RD_INFO   0x00 //           0         52
#define CMD_RD_DISP   0x01 //           0         12
#define CMD_RD_CAL    0x10 //           0         23
#define CMD_RD_EEPROM 0x1a //           1         1-64
#define CMD_RD_HARM   0x1b //           0         50
#define CMD_IN_CMODE  0x80 //           0         0
#define CMD_WR_FCODE  0x85 //           1         0
#define CMD_WR_RCODE  0x87 //           1         0
#define CMD_WR_EEPROM 0x8a //           1-64      0
#define CMD_OUT_CMODE 0x8f //           0         0

static int tty_config(int fd, speed_t speed);
static int appa208_write(int appa208_fd, const uint8_t *cmd, int cmd_length);
static int appa208_read(int appa208_fd, uint8_t *buffer, int buffer_size);

int appa208_open(const char *tty_filename, int *appa208_fd)
{
	int ret = 0;
	int fd;
	int r;

	fd = open(tty_filename, O_RDWR | O_NOCTTY);
	if (fd == -1)
	{
		fprintf(stderr, "# E: unable to open APPA208 [%s] (%s)\n", tty_filename, strerror(errno));
		ret = -1;
		goto appa208_open_exit;
	}

	r = tty_config(fd, B9600);
	if (r == -1)
	{
		ret = -2;
		close(fd);
		goto appa208_open_exit;
	}

	*appa208_fd = fd;

	appa208_open_exit:
	return ret;
}

int appa208_close(int appa208_fd)
{
	int ret = 0;
	int r;

	r = close(appa208_fd);
	if (r == -1)
	{
		fprintf(stderr, "# E: unable to close APPA208 (%s)\n", strerror(errno));
		ret = -1;
		goto appa208_close_exit;
	}

	appa208_close_exit:
	return ret;
}

int appa208_read_info(int appa208_fd, char *model, char *serial, uint16_t *model_id, uint16_t *fw_version)
{
	int ret = 0;
	int r;

	const uint8_t cmd[] = {0x55, 0x55, CMD_RD_INFO, 0x00, 0xaa};
	r = appa208_write(appa208_fd, cmd, 5);
	if (r < 0)
	{
		ret = r;
		goto appa208_get_idn_exit;
	}

	uint8_t buffer[57] = {0};
	r = appa208_read(appa208_fd, buffer, 57);
	if (r < 0)
	{
		ret = r;
		goto appa208_get_idn_exit;
	}

	// check error in the answer
	int check_err;
	uint8_t checksum = 0;

	for (int i = 0; i < 56; ++i)
	{
		checksum += buffer[i];
	}

	check_err =
		(buffer[0] != 0x55) ||
		(buffer[1] != 0x55) ||
		(buffer[2] != CMD_RD_INFO) ||
		(buffer[3] != 0x34) ||
		(buffer[56] != checksum);

	if (check_err)
	{
		ret = -1;
		goto appa208_get_idn_exit;
	}

	memcpy(model,  buffer+4, 32);
	memcpy(serial, buffer+4+32, 16);

	*model_id = buffer[4+49];
	*model_id <<= 8;
	*model_id |= buffer[4+48];

	*fw_version = buffer[4+51];
	*fw_version <<= 8;
	*fw_version |= buffer[4+50];

	appa208_get_idn_exit:
	return ret;
}

static int tty_config(int fd, speed_t speed)
{
	struct termios serial_port_settings;
	int r;

	// Get the current attributes of the Serial port
	r = tcgetattr(fd, &serial_port_settings);
	if (r == -1)
	{
		fprintf(stderr, "# E: unable to get terminal settings (%s)\n", strerror(errno));
		return -1;
	}

	r = cfsetispeed(&serial_port_settings, speed);
	if (r == -1)
	{
		fprintf(stderr, "# E: unable to set terminal input speed (%s)\n", strerror(errno));
		return -1;
	}

	r = cfsetospeed(&serial_port_settings, speed);
	if (r == -1)
	{
		fprintf(stderr, "# E: unable to set terminal output speed (%s)\n", strerror(errno));
		return -1;
	}

	serial_port_settings.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN);
	serial_port_settings.c_lflag &= ~(ECHOCTL | ECHOKE);

	serial_port_settings.c_oflag &= ~(OPOST | ONLCR | OCRNL);

	serial_port_settings.c_iflag &= ~(INLCR | IGNCR | ICRNL | IGNBRK);
	serial_port_settings.c_iflag &= ~(IUCLC | PARMRK);
	serial_port_settings.c_iflag &= ~(INPCK | ISTRIP);
	serial_port_settings.c_iflag &= ~(IXON | IXOFF | IXANY);

	serial_port_settings.c_cflag |= (CREAD | CLOCAL);
	serial_port_settings.c_cflag |= (CS8);
	serial_port_settings.c_cflag &= ~(CSTOPB);
	serial_port_settings.c_cflag &= ~(PARENB | PARODD | CMSPAR);
	serial_port_settings.c_cflag &= ~(CRTSCTS);
	// serial_port_settings.c_cflag &= ~(CNEW_RTSCTS);

	serial_port_settings.c_cc[VMIN] = 1; // Read at least 255 characters
	serial_port_settings.c_cc[VTIME] = 0; // Wait indefinetly

	r = tcsetattr(fd, TCSANOW, &serial_port_settings);
	if (r == -1)
	{
		fprintf(stderr, "# E: unable to setup terminal settings (%s)\n", strerror(errno));
		return -1;
	}

	// Discards old data in rx and tx buffer
	r = tcflush(fd, TCIOFLUSH);
	if (r == -1)
	{
		fprintf(stderr, "# E: unable to flush input of terminal (%s)\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int appa208_write(int appa208_fd, const uint8_t *cmd, int cmd_length)
{
	int ret = 0;
	int n;

	n = write(appa208_fd, cmd, cmd_length);
	if (n == -1)
	{
		fprintf(stderr, "# E: unable to write to APPA208 (%s)\n", strerror(errno));
		ret = -1;
		goto appa208_write_exit;
	}
	else if (n != cmd_length)
	{
		fprintf(stderr, "# E: unable to write full command to APPA208 (%d bytes written)\n", n);
		ret = -2;
		goto appa208_write_exit;
	}

	ret = n;

	appa208_write_exit:
	return ret;
}

static int appa208_read(int appa208_fd, uint8_t *buffer, int buffer_size)
{
	int ret = 0;
	int n;
	int i;

	i = 0;
	while (i < buffer_size)
	{
		n = read(appa208_fd, &buffer[i], 1);
		if (n == -1)
		{
			fprintf(stderr, "# E: unable to read from APPA208 (%s)\n", strerror(errno));
			ret = -1;
			goto appa208_read_exit;
		}
		else if (n != 1)
		{
			fprintf(stderr, "# E: unable to read a symbol from APPA208 (%d)\n", n);
			ret = -2;
			goto appa208_read_exit;
		}

		i++;

		if (buffer[i-1] == '\n')
		{
			break;
		}
	}

	appa208_read_exit:
	if ((i > 0) && (ret == 0))
	{
		ret = i;
	}

	return ret;
}