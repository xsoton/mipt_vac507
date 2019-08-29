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

static const char *ftcodes[] = {"Manual Test", "Auto Test"};
static const char *fcodes[] = {
	"None", "AC V", "DC V", "AC mV", "DC mV", "Ohm", "Continuity", "Diode", "Cap", "AC A",
	"DC A", "AC mA", "DC mA", "°C", "°F", "Frequency", "Duty", "Hz (V)", "Hz (mV)", "Hz (A)",
	"Hz (mA)", "AC+DC (V)", "AC+DC (mV)", "AC+DC (A)", "AC+DC (mA)", "LPF (V)", "LPF (mV)",
	"LPF (A)", "LPF (mA)", "AC uA", "DC uA", "DC A out", "DC A out (Slow Linear)",
	"DC A out (Fast Linear)", "DC A out (Slow Step)", "DC A out (Fast Step)", "LoopPower",
	"250ohmHart", "VoltSense", "PeakHold (V)", "PeakHold (mV)", "PeakHold (A)", "PeakHold (mA)",
	"LoZ AC V", "LoZ DC V", "LoZ AC+DC (V)", "LoZ LPF (V)", "LoZ Hz (V)", "LoZ PeakHold (V)",
	"Battery", "AC W", "DC W", "PF", "Flex AC A", "Flex LPF (A)", "Flex PeakHold (A)",
	"Flex Hz (A)", "Vharm", "Inrush", "Aharm", "Flex Inrush", "Flex Aharm", "PeakHold (uA)"};

static const char *words[] = {
	"Space", "Full", "Beep", "APO", "b.Lit", "HAZ", "On", "Off", "Reset", "Start", "View",
	"Pause", "Fuse", "Probe", "dEF", "Clr", "Er", "Er1", "Er2", "Er3", "Dash", "Dash1",
	"Test", "Dash2", "bAtt", "diSLt", "noiSE", "FiLtr", "PASS", "null", "0-20", "4-20",
	"RATE", "SAVE", "LOAd", "YES", "SEnd", "Ahold", "Auto", "Cntin", "CAL", "Version",
	"OL (not use)", "FULL", "HALF", "Lo", "Hi", "digit", "rdy", "dISC", "outF", "OLA",
	"OLV", "OLVA", "bAd", "TEMP"};

static const char *units[] = {
	"None", "V", "mV", "A", "mA", "dB", "dBm", "mF", "uF", "nF", "GΩ", "MΩ", "kΩ", "Ω", "%%",
	"MHz", "kHz", "Hz", "°C", "°F", "sec", "ms", "us", "ns", "uA", "min", "kW", "PF"};

static const char *ols[] = {"Not Overload", "Overload"};

static const char *dcs[] = {
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

int appa208_read_info(int appa208_fd, char *model, char *serial, uint16_t *model_id, uint16_t *fw_version)
{
	int ret = 0;
	int r;

	const uint8_t cmd[] = {0x55, 0x55, CMD_RD_INFO, 0x00, 0xaa};
	r = appa208_write(appa208_fd, cmd, 5);
	if (r < 0)
	{
		ret = r;
		goto appa208_read_info_exit;
	}

	uint8_t buffer[57] = {0};
	r = appa208_read(appa208_fd, buffer, 57);
	if (r < 0)
	{
		ret = r;
		goto appa208_read_info_exit;
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
		goto appa208_read_info_exit;
	}

	memcpy(model,  buffer+4, 32);
	memcpy(serial, buffer+4+32, 16);

	*model_id = buffer[4+49];
	*model_id <<= 8;
	*model_id |= buffer[4+48];

	*fw_version = buffer[4+51];
	*fw_version <<= 8;
	*fw_version |= buffer[4+50];

	appa208_read_info_exit:
	return ret;
}

int appa208_read_disp(int appa208_fd, double *mvalue, double *svalue, disp_status_t *status)
{
	int ret = 0;
	int r;

	const uint8_t cmd[] = {0x55, 0x55, CMD_RD_DISP, 0x00, 0xab};
	r = appa208_write(appa208_fd, cmd, 5);
	if (r < 0)
	{
		ret = r;
		goto appa208_read_disp_exit;
	}

	uint8_t buffer[17] = {0};
	r = appa208_read(appa208_fd, buffer, 17);
	if (r < 0)
	{
		ret = r;
		goto appa208_read_disp_exit;
	}

	for (int i = 0; i < 17; ++i)
	{
		printf("%02x ", buffer[i]);
	}
	putchar('\n');

	// check error in the answer
	int check_err;
	uint8_t checksum = 0;

	for (int i = 0; i < 16; ++i)
	{
		checksum += buffer[i];
	}

	check_err =
		(buffer[0] != 0x55) ||
		(buffer[1] != 0x55) ||
		(buffer[2] != CMD_RD_DISP) ||
		(buffer[3] != 0x0c) ||
		(buffer[16] != checksum);

	if (check_err)
	{
		ret = -1;
		goto appa208_read_disp_exit;
	}

	// parse answer
	status->auto_test = buffer[4+0] >> 7;
	status->func_code = buffer[4+0] & 0x7f;

	status->auto_range = buffer[4+1] >> 7;
	// TODO: complete range parse

	status->main_value = buffer[4+2+2];
	status->main_value <<= 8;
	status->main_value |= buffer[4+2+1];
	status->main_value <<= 8;
	status->main_value |= buffer[4+2+0];
	status->main_unit = buffer[4+2+3] >> 3;
	status->main_dot = buffer[4+2+3] & 0x07;
	status->main_ol = buffer[4+2+3+1] >> 7;
	status->main_dc = buffer[4+2+3+1] & 0x7f;

	status->sub_value = buffer[4+2+5+2];
	status->sub_value <<= 8;
	status->sub_value |= buffer[4+2+5+1];
	status->sub_value <<= 8;
	status->sub_value |= buffer[4+2+5+0];
	status->sub_unit = buffer[4+2+5+3] >> 3;
	status->sub_dot = buffer[4+2+5+3] & 0x07;
	status->sub_ol = buffer[4+2+5+3+1] >> 7;
	status->sub_dc = buffer[4+2+5+3+1] & 0x7f;

	printf("auto_test = 0x%x  (%s)\n", status->auto_test, ftcodes[status->auto_test]);
	printf("func_code      = 0x%x  (%s)\n", status->func_code, fcodes[status->func_code]);
	// printf("range_code     = 0x%x\n", range_code);
	printf("main_value     = 0x%x (%d)", status->main_value, status->main_value);
	if ((status->main_value >= 0x700000) && (status->main_value < 0x800000))
	{
		printf(" (%s)\n", words[status->main_value-0x700000]);
	}
	else
	{
		printf("\n");
	}
	printf("main_unit      = 0x%x  (%s)\n", status->main_unit, units[status->main_unit]);
	printf("main_dot       = 0x%x\n", status->main_dot);
	printf("main_ol        = 0x%x (%s)\n", status->main_ol, ols[status->main_ol]);
	printf("main_dc        = 0x%x (%s)\n", status->main_dc, dcs[status->main_dc]);

	printf("sub_value      = 0x%x (%d)", status->sub_value, status->sub_value);
	if ((status->sub_value >= 0x700000) && (status->sub_value < 0x800000))
	{
		printf(" (%s)\n", words[status->sub_value-0x700000]);
	}
	else
	{
		printf("\n");
	}
	printf("sub_unit       = 0x%x (%s)\n", status->sub_unit, units[status->sub_unit]);
	printf("sub_dot        = 0x%x\n", status->sub_dot);
	printf("sub_ol         = 0x%x (%s)\n", status->sub_ol, ols[status->sub_ol]);
	printf("sub_dc         = 0x%x (%s)\n", status->sub_dc, dcs[status->sub_dc]);

	if ((status->main_value >= 0x700000) && (status->main_value < 0x800000))
	{
		fprintf(stdout, "main word = \"%s\"\n", words[status->main_value-0x700000]);
		ret = -2;
		goto appa208_read_disp_exit;
	}

	*mvalue = status->main_value;
	while(status->main_dot > 0)
	{
		*mvalue /= 10;
		status->main_dot--;
	}

	printf("mvalue = %lf\n", *mvalue);

	if ((status->sub_value >= 0x700000) && (status->sub_value < 0x800000))
	{
		fprintf(stdout, "sub word = \"%s\"\n", words[status->sub_value-0x700000]);
		ret = -2;
		goto appa208_read_disp_exit;
	}

	*svalue = status->sub_value;
	while(status->sub_dot > 0)
	{
		*svalue /= 10;
		status->sub_dot--;
	}

	printf("svalue = %lf\n", *svalue);

	switch (status->main_unit)
	{
		case U_None:
			break;
		case U_V:
			break;
		case U_mV:
			status->main_unit = U_V;
			*mvalue /= 1e3;
			break;
		case U_A:
			break;
		case U_mA:
			status->main_unit = U_A;
			*mvalue /= 1e3;
			break;
		case U_dB:
			break;
		case U_dBm:
			status->main_unit = U_dB;
			*mvalue /= 1e3;
			break;
		case U_mF:
			status->main_unit = U_F;
			*mvalue /= 1e3;
			break;
		case U_uF:
			status->main_unit = U_F;
			*mvalue /= 1e6;
			break;
		case U_nF:
			status->main_unit = U_F;
			*mvalue /= 1e9;
			break;
		case U_GOhm:
			status->main_unit = U_Ohm;
			*mvalue *= 1e9;
			break;
		case U_MOhm:
			status->main_unit = U_Ohm;
			*mvalue *= 1e6;
			break;
		case U_kOhm:
			status->main_unit = U_Ohm;
			*mvalue *= 1e3;
			break;
		case U_Ohm:
			break;
		case U_percent:
			break;
		case U_MHz:
			status->main_unit = U_Hz;
			*mvalue *= 1e6;
			break;
		case U_kHz:
			status->main_unit = U_Hz;
			*mvalue *= 1e3;
			break;
		case U_Hz:
			break;
		case U_Cel:
			break;
		case U_Far:
			break;
		case U_sec:
			break;
		case U_ms:
			status->main_unit = U_sec;
			*mvalue /= 1e3;
			break;
		case U_us:
			status->main_unit = U_sec;
			*mvalue /= 1e6;
			break;
		case U_ns:
			status->main_unit = U_sec;
			*mvalue /= 1e9;
			break;
		case U_uA:
			status->main_unit = U_A;
			*mvalue /= 1e6;
			break;
		case U_min:
			status->main_unit = U_sec;
			*mvalue *= 60;
			break;
		case U_kW:
			break;
		case U_PF:
			status->main_unit = U_F;
			*mvalue /= 1e12;
			break;
	}

	switch (status->sub_unit)
	{
		case U_None:
			break;
		case U_V:
			break;
		case U_mV:
			status->sub_unit = U_V;
			*svalue /= 1e3;
			break;
		case U_A:
			break;
		case U_mA:
			status->sub_unit = U_A;
			*svalue /= 1e3;
			break;
		case U_dB:
			break;
		case U_dBm:
			status->sub_unit = U_dB;
			*svalue /= 1e3;
			break;
		case U_mF:
			status->sub_unit = U_F;
			*svalue /= 1e3;
			break;
		case U_uF:
			status->sub_unit = U_F;
			*svalue /= 1e6;
			break;
		case U_nF:
			status->sub_unit = U_F;
			*svalue /= 1e9;
			break;
		case U_GOhm:
			status->sub_unit = U_Ohm;
			*svalue *= 1e9;
			break;
		case U_MOhm:
			status->sub_unit = U_Ohm;
			*svalue *= 1e6;
			break;
		case U_kOhm:
			status->sub_unit = U_Ohm;
			*svalue *= 1e3;
			break;
		case U_Ohm:
			break;
		case U_percent:
			break;
		case U_MHz:
			status->sub_unit = U_Hz;
			*svalue *= 1e6;
			break;
		case U_kHz:
			status->sub_unit = U_Hz;
			*svalue *= 1e3;
			break;
		case U_Hz:
			break;
		case U_Cel:
			break;
		case U_Far:
			break;
		case U_sec:
			break;
		case U_ms:
			status->sub_unit = U_sec;
			*svalue /= 1e3;
			break;
		case U_us:
			status->sub_unit = U_sec;
			*svalue /= 1e6;
			break;
		case U_ns:
			status->sub_unit = U_sec;
			*svalue /= 1e9;
			break;
		case U_uA:
			status->sub_unit = U_A;
			*svalue /= 1e6;
			break;
		case U_min:
			status->sub_unit = U_sec;
			*svalue *= 60;
			break;
		case U_kW:
			break;
		case U_PF:
			status->sub_unit = U_F;
			*svalue /= 1e12;
			break;
	}

	appa208_read_disp_exit:
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