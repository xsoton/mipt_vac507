#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "qj3003p.h"

static int tty_config(int fd, speed_t speed);
static int qj3003p_write(int qj3003p_fd, const char *cmd, int cmd_length);
static int qj3003p_read(int qj3003p_fd, char *str, int max_str_length);

int qj3003p_open(const char *tty_filename, int *qj3003p_fd)
{
	int ret = 0;
	int fd;
	int r;

	fd = open(tty_filename, O_RDWR | O_NOCTTY);
	if (fd == -1)
	{
		fprintf(stderr, "# E: unable to open QJ3003P [%s] (%s)\n", tty_filename, strerror(errno));
		ret = -1;
		goto qj3003p_open_exit;
	}

	r = tty_config(fd, B9600);
	if (r == -1)
	{
		ret = -2;
		close(fd);
		goto qj3003p_open_exit;
	}

	*qj3003p_fd = fd;

	qj3003p_open_exit:
	return ret;
}

int qj3003p_close(int qj3003p_fd)
{
	int ret = 0;
	int r;

	r = close(qj3003p_fd);
	if (r == -1)
	{
		fprintf(stderr, "# E: unable to close QJ3003P (%s)\n", strerror(errno));
		ret = -1;
		goto qj3003p_close_exit;
	}

	qj3003p_close_exit:
	return ret;
}

int qj3003p_get_idn(int qj3003p_fd, char *str, int max_str_length)
{
	int ret = 0;
	int r;

	const char *cmd = "*IDN?\\n";
	r = qj3003p_write(qj3003p_fd, cmd, strlen(cmd));
	if (r < 0)
	{
		ret = r;
		goto qj3003p_get_idn_exit;
	}

	r = qj3003p_read(qj3003p_fd, str, max_str_length);
	if (r < 0)
	{
		ret = r;
		goto qj3003p_get_idn_exit;
	}

	qj3003p_get_idn_exit:
	return ret;
}

int qj3003p_set_voltage(int qj3003p_fd, double voltage)
{
	int ret = 0;
	int r;
	int n;

	char buffer[20] = {0};
	const int buffer_size = 20;
	int buffer_length = 0;


	if ((voltage < 0.0) || (voltage > 31.0))
	{
		ret = -1;
		goto qj3003p_set_voltage_exit;
	}

	n = snprintf(buffer, buffer_size, "VSET1:%05.2f\\n", voltage);
	if (n < 0)
	{
		fprintf(stderr, "# E: Unable to format command \"%s\" (%s)\n", buffer, strerror(errno));
		ret = -2;
		goto qj3003p_set_voltage_exit;
	}

	buffer_length = n;

	r = qj3003p_write(qj3003p_fd, buffer, buffer_length);
	if (r < 0)
	{
		ret = r;
		goto qj3003p_set_voltage_exit;
	}

	// usleep(100000);

	qj3003p_set_voltage_exit:
	return ret;
}

int qj3003p_get_voltage(int qj3003p_fd, double *voltage)
{
	int ret = 0;
	int r;

	char buffer[20] = {0};
	const int buffer_size = 20;

	const char *cmd = "VOUT1?\\n";
	r = qj3003p_write(qj3003p_fd, cmd, strlen(cmd));
	if (r < 0)
	{
		ret = r;
		goto qj3003p_get_voltage_exit;
	}

	r = qj3003p_read(qj3003p_fd, buffer, buffer_size);
	if (r < 0)
	{
		ret = r;
		goto qj3003p_get_voltage_exit;
	}

	*voltage = atof(buffer);

	qj3003p_get_voltage_exit:
	return ret;
}

int qj3003p_set_current(int qj3003p_fd, double current)
{
	int ret = 0;
	int r;
	int n;

	char buffer[20] = {0};
	const int buffer_size = 20;
	int buffer_length = 0;


	if ((current < 0.0) || (current > 3.2))
	{
		ret = -1;
		goto qj3003p_set_current_exit;
	}

	n = snprintf(buffer, buffer_size, "ISET1:%.3f\\n", current);
	if (n < 0)
	{
		fprintf(stderr, "# E: Unable to format command \"%s\" (%s)\n", buffer, strerror(errno));
		ret = -2;
		goto qj3003p_set_current_exit;
	}

	buffer_length = n;

	r = qj3003p_write(qj3003p_fd, buffer, buffer_length);
	if (r < 0)
	{
		ret = r;
		goto qj3003p_set_current_exit;
	}

	// usleep(100000);

	qj3003p_set_current_exit:
	return ret;
}

int qj3003p_get_current(int qj3003p_fd, double *current)
{
	int ret = 0;
	int r;

	char buffer[20] = {0};
	const int buffer_size = 20;

	const char *cmd = "IOUT1?\\n";
	r = qj3003p_write(qj3003p_fd, cmd, strlen(cmd));
	if (r < 0)
	{
		ret = r;
		goto qj3003p_get_current_exit;
	}

	r = qj3003p_read(qj3003p_fd, buffer, buffer_size);
	if (r < 0)
	{
		ret = r;
		goto qj3003p_get_current_exit;
	}

	*current = atof(buffer);

	qj3003p_get_current_exit:
	return ret;
}

int qj3003p_set_output(int qj3003p_fd, int output)
{
	int ret = 0;
	int r;
	int n;

	char buffer[20] = {0};
	const int buffer_size = 20;
	int buffer_length = 0;

	if ((output != 0) && (output != 1))
	{
		ret = -1;
		goto qj3003p_set_output_exit;
	}

	n = snprintf(buffer, buffer_size, "OUTPUT%d\\n", output);
	if (n < 0)
	{
		fprintf(stderr, "# E: Unable to format command \"%s\" (%s)\n", buffer, strerror(errno));
		ret = -2;
		goto qj3003p_set_output_exit;
	}

	buffer_length = n;

	r = qj3003p_write(qj3003p_fd, buffer, buffer_length);
	if (r < 0)
	{
		ret = r;
		goto qj3003p_set_output_exit;
	}

	// usleep(100000);

	qj3003p_set_output_exit:
	return ret;
}

int qj3003p_get_status(int qj3003p_fd, int *output)
{
	int ret = 0;
	int r;

	char buffer[20] = {0};
	const int buffer_size = 20;

	const char *cmd = "STATUS?\\n";
	r = qj3003p_write(qj3003p_fd, cmd, strlen(cmd));
	if (r < 0)
	{
		ret = r;
		goto qj3003p_get_status_exit;
	}

	r = qj3003p_read(qj3003p_fd, buffer, buffer_size);
	if (r < 0)
	{
		ret = r;
		goto qj3003p_get_status_exit;
	}

	// printf("status = \"%s\"\n", buffer);
	*output = (buffer[1] == '0') ? 0 : 1;

	qj3003p_get_status_exit:
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

static int qj3003p_write(int qj3003p_fd, const char *cmd, int cmd_length)
{
	int ret = 0;
	int n;

	n = write(qj3003p_fd, cmd, cmd_length);
	if (n == -1)
	{
		fprintf(stderr, "# E: unable to write to QJ3003P (%s)\n", strerror(errno));
		ret = -1;
		goto qj3003p_write_exit;
	}
	else if (n != cmd_length)
	{
		fprintf(stderr, "# E: unable to write full command to QJ3003P (%d bytes written)\n", n);
		ret = -2;
		goto qj3003p_write_exit;
	}

	ret = n;

	qj3003p_write_exit:
	return ret;
}

static int qj3003p_read(int qj3003p_fd, char *str, int max_str_length)
{
	int ret = 0;
	int n;
	int i;

	i = 0;
	while (i < max_str_length)
	{
		n = read(qj3003p_fd, &str[i], 1);
		if (n == -1)
		{
			fprintf(stderr, "# E: unable to read from QJ3003P (%s)\n", strerror(errno));
			ret = -1;
			goto qj3003p_read_exit;
		}
		else if (n != 1)
		{
			fprintf(stderr, "# E: unable to read a symbol from QJ3003P (%d)\n", n);
			ret = -2;
			goto qj3003p_read_exit;
		}

		i++;

		if (str[i-1] == '\n')
		{
			break;
		}
	}

	qj3003p_read_exit:
	if ((i > 0) && (ret == 0))
	{
		ret = i;
	}

	return ret;
}

void qj3003p_delay()
{
	usleep(100000);
}