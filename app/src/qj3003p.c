#include <stdio.h>
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

	const char *cmd = "*IDN?\\r\\n";
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

// int qj3003p_set_voltage(int qj3003p_fd, double voltage);
// int qj3003p_get_voltage(int qj3003p_fd, double *voltage);
// int qj3003p_set_current(int qj3003p_fd, double current);
// int qj3003p_get_current(int qj3003p_fd, double *current);
// int qj3003p_set_output(int qj3003p_fd, int output);
// int qj3003p_get_status(int qj3003p_fd, int *status);

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

	// serial_port_settings.c_lflag &= ~ICANON; // Cannonical mode
	// serial_port_settings.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ISIG);

	// serial_port_settings.c_cflag &= ~PARENB;        // Disables the Parity Enable bit(PARENB)
	// serial_port_settings.c_cflag &= ~CSTOPB;        // CSTOPB = 2 Stop bits, here it's cleared so 1 Stop bit
	// serial_port_settings.c_cflag &= ~CSIZE;         // Clears the mask for setting the data size
	// serial_port_settings.c_cflag |=  CS8;           // Set the data bits = 8
	// serial_port_settings.c_cflag &= ~CRTSCTS;       // No Hardware flow Control
	// serial_port_settings.c_cflag |= CREAD | CLOCAL; // Enable receiver, Ignore Modem Control lines

	// serial_port_settings.c_iflag &= ~(IXON | IXOFF | IXANY);         // Disable XON/XOFF flow control both i/p and o/p
	// serial_port_settings.c_oflag &= ~OPOST; // No Output Processing

	// serial_port_settings.c_cc[VMIN] = 0; // Read at least 255 characters
	// serial_port_settings.c_cc[VTIME] = 10; // Wait indefinetly


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