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

static const char *appa208_ftcodes[] = {"Manual Test", "Auto Test"};
static const char *appa208_fcodes[] = {
	"None", "AC V", "DC V", "AC mV", "DC mV", "Ohm", "Continuity", "Diode", "Cap", "AC A",
	"DC A", "AC mA", "DC mA", "°C", "°F", "Frequency", "Duty", "Hz (V)", "Hz (mV)", "Hz (A)",
	"Hz (mA)", "AC+DC (V)", "AC+DC (mV)", "AC+DC (A)", "AC+DC (mA)", "LPF (V)", "LPF (mV)",
	"LPF (A)", "LPF (mA)", "AC uA", "DC uA", "DC A out", "DC A out (Slow Linear)",
	"DC A out (Fast Linear)", "DC A out (Slow Step)", "DC A out (Fast Step)", "LoopPower",
	"250ohmHart", "VoltSense", "PeakHold (V)", "PeakHold (mV)", "PeakHold (A)", "PeakHold (mA)",
	"LoZ AC V", "LoZ DC V", "LoZ AC+DC (V)", "LoZ LPF (V)", "LoZ Hz (V)", "LoZ PeakHold (V)",
	"Battery", "AC W", "DC W", "PF", "Flex AC A", "Flex LPF (A)", "Flex PeakHold (A)",
	"Flex Hz (A)", "Vharm", "Inrush", "Aharm", "Flex Inrush", "Flex Aharm", "PeakHold (uA)"};

static const char *appa208_words[] = {
	"Space", "Full", "Beep", "APO", "b.Lit", "HAZ", "On", "Off", "Reset", "Start", "View",
	"Pause", "Fuse", "Probe", "dEF", "Clr", "Er", "Er1", "Er2", "Er3", "Dash", "Dash1",
	"Test", "Dash2", "bAtt", "diSLt", "noiSE", "FiLtr", "PASS", "null", "0-20", "4-20",
	"RATE", "SAVE", "LOAd", "YES", "SEnd", "Ahold", "Auto", "Cntin", "CAL", "Version",
	"OL (not use)", "FULL", "HALF", "Lo", "Hi", "digit", "rdy", "dISC", "outF", "OLA",
	"OLV", "OLVA", "bAd", "TEMP"};

static const char *appa208_units[] = {
	"None", "V", "mV", "A", "mA", "dB", "dBm", "mF", "uF", "nF", "GΩ", "MΩ", "kΩ", "Ω", "%%",
	"MHz", "kHz", "Hz", "°C", "°F", "sec", "ms", "us", "ns", "uA", "min", "kW", "PF", "F", "W"};

static const char *appa208_ols[] = {"Not Overload", "Overload"};

static const char *appa208_dcs[] = {
	"Measuring data", "Frequency", "Cycle", "Duty", "Memory Stamp", "Memory Save", "Memory Load",
	"Log Save", "Log Load", "Log Rate", "REL Δ", "REL %", "REL Reference", "Maximum", "Minimum",
	"Average", "Peak Hold Max", "Peak Hold Min", "dBm", "dB", "Auto Hold", "Setup", "Log Stamp",
	"Log Max", "Log Min", "Log TP", "Hold", "Current Output", "CurOut 0-20mA %%", "CurOut 4-20mA %%"};

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

int appa208_read_info(int appa208_fd, appa208_info_t *info)
{
	int ret = 0;
	int r;

	const uint8_t cmd[] = {0x55, 0x55, CMD_RD_INFO, 0x00, 0xaa};
	r = appa208_write(appa208_fd, cmd, 5);
	if (r < 0)
	{
		ret = -1;
		goto appa208_read_info_exit;
	}

	appa208_rd_info_ans_packet_t packet;
	r = appa208_read(appa208_fd, packet.buffer, sizeof(packet.buffer));
	if (r < 0)
	{
		ret = -2;
		goto appa208_read_info_exit;
	}

	uint8_t checksum = 0;

	for (size_t i = 0; i < sizeof(packet.buffer) - 1; ++i)
	{
		checksum += packet.buffer[i];
	}

	// for (size_t i = 0; i < sizeof(packet.buffer); ++i)
	// {
	// 	printf("0x%02x ", packet.buffer[i]);
	// }
	// printf("\n");

	int check_err =
		(packet.answer.start0      != 0x55) ||
		(packet.answer.start1      != 0x55) ||
		(packet.answer.cmd_code    != CMD_RD_INFO) ||
		(packet.answer.data_length != 0x34) ||
		(packet.answer.checksum    != checksum);

	if (check_err)
	{
		ret = -3;
		goto appa208_read_info_exit;
	}

	memcpy(info, &packet.answer.data, sizeof(appa208_info_t));

	appa208_read_info_exit:
	return ret;
}

int appa208_read_disp(int appa208_fd, appa208_disp_t *disp)
{
	int ret = 0;
	int r;

	const uint8_t cmd[] = {0x55, 0x55, CMD_RD_DISP, 0x00, 0xab};
	r = appa208_write(appa208_fd, cmd, 5);
	if (r < 0)
	{
		ret = -1;
		goto appa208_read_disp_exit;
	}

	appa208_rd_disp_ans_packet_t packet;
	r = appa208_read(appa208_fd, packet.buffer, sizeof(packet.buffer));
	if (r < 0)
	{
		ret = -2;
		goto appa208_read_disp_exit;
	}

	uint8_t checksum = 0;
	for (size_t i = 0; i < sizeof(packet.buffer) - 1; ++i)
	{
		checksum += packet.buffer[i];
	}

	// for (size_t i = 0; i < sizeof(packet.buffer); ++i)
	// {
	// 	printf("0x%02x ", packet.buffer[i]);
	// }
	// printf("\n");

	int check_err =
		(packet.answer.start0      != 0x55) ||
		(packet.answer.start1      != 0x55) ||
		(packet.answer.cmd_code    != CMD_RD_DISP) ||
		(packet.answer.data_length != 0x0c) ||
		(packet.answer.checksum    != checksum);

	if (check_err)
	{
		ret = -3;
		goto appa208_read_disp_exit;
	}

	memcpy(disp, &packet.answer.data, sizeof(appa208_disp_t));

	appa208_read_disp_exit:
	return ret;
}

static const double appa208_munit[] = {
	1, 1, 1e-3, 1, 1e-3, 1, 1e-3, 1e-3, 1e-6, 1e-9, 1e9, 1e6, 1e3, 1, 1,
	1e6, 1e3, 1, 1, 1, 1, 1e-3, 1e-6, 1e-9, 1e-6, 60, 1e3, 1e-12, 1, 1
};

static const unit_t appa208_runit[] = {
	U_None, U_V, U_V, U_A, U_A, U_dB, U_dB, U_F, U_F, U_F, U_Ohm, U_Ohm, U_Ohm,	U_Ohm, U_percent,
	U_Hz, U_Hz, U_Hz, U_Cel, U_Far, U_sec, U_sec, U_sec, U_sec, U_A, U_sec,	U_W, U_F, U_F, U_W
};

double appa208_get_value(appa208_disp_data_t *disp_data)
{
	double value;
	uint32_t v;

	v = disp_data->reading[2];
	v <<= 8;
	v += disp_data->reading[1];
	v <<= 8;
	v += disp_data->reading[0];

	value = (double)v;

	switch(disp_data->dot)
	{
		case 0x04: value *= 0.0001; break;
		case 0x03: value *= 0.001;  break;
		case 0x02: value *= 0.01;   break;
		case 0x01: value *= 0.1;    break;
	}

	return value * appa208_munit[disp_data->unit];
}

unit_t appa208_get_unit(appa208_disp_data_t *disp_data)
{
	return appa208_runit[disp_data->unit];
}

int appa208_get_overload(appa208_disp_data_t *disp_data)
{
	return (int)disp_data->overload;
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

const char *appa208_str_ftcode(uint8_t ftcode)
{
	return appa208_ftcodes[ftcode];
}

const char *appa208_str_fcode(uint8_t fcode)
{
	return appa208_fcodes[fcode];
}

const char *appa208_str_word(uint16_t wcode)
{
	return appa208_words[wcode];
}

const char *appa208_str_unit(unit_t unit)
{
	return appa208_units[unit];
}

const char *appa208_str_overload(uint8_t overload)
{
	return appa208_ols[overload];
}

const char *appa208_str_data_content(uint8_t data_content)
{
	return appa208_dcs[data_content];
}
