#include <stdint.h>
#include <inttypes.h>

typedef enum {
	U_None = 0, U_V, U_mV, U_A, U_mA, U_dB, U_dBm, U_mF, U_uF, U_nF, U_GOhm, U_MOhm, U_kOhm, U_Ohm,
	U_percent, U_MHz, U_kHz, U_Hz, U_Cel, U_Far, U_sec, U_ms, U_us, U_ns, U_uA, U_min, U_kW, U_PF, U_F
} unit_t;

typedef struct
{
	uint8_t auto_test;
	uint8_t func_code;
	uint8_t auto_range;
	uint8_t range_unit;
	double range;
	uint32_t main_value;
	unit_t main_unit;
	uint8_t main_dot;
	uint8_t main_ol;
	uint8_t main_dc;
	uint32_t sub_value;
	unit_t sub_unit;
	uint8_t sub_dot;
	uint8_t sub_ol;
	uint8_t sub_dc;
} disp_status_t;

int appa208_open(const char *tty_filename, int *appa208_fd);
int appa208_close(int appa208_fd);
int appa208_read_info(int appa208_fd, char *model, char *serial, uint16_t *model_id, uint16_t *fw_version);
int appa208_read_disp(int appa208_fd, double *mvalue, double *svalue, disp_status_t *status);
