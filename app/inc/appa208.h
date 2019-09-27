#include <stdint.h>
#include <inttypes.h>

// information
typedef struct __attribute__((__packed__))
{
	char model_name[32];
	char serial_name[16];
	char model_id[2];
	char fw_version[2];
} appa208_info_t;

typedef struct __attribute__((__packed__))
{
	uint8_t start0;
	uint8_t start1;
	uint8_t cmd_code;
	uint8_t data_length;
	appa208_info_t data;
	uint8_t checksum;
} appa208_rd_info_ans_t;

typedef union
{
	uint8_t buffer[57];
	appa208_rd_info_ans_t answer;
} appa208_rd_info_ans_packet_t;


// display
typedef struct __attribute__((__packed__))
{
	uint8_t reading[3];
	uint8_t dot:3;
	uint8_t unit:5;
	uint8_t data_content:7;
	uint8_t overload:1;
} appa208_disp_data_t;

typedef struct __attribute__((__packed__))
{
	uint8_t fcode:7;
	uint8_t auto_test:1;
	uint8_t rcode:7;
	uint8_t auto_range:1;
	appa208_disp_data_t mdata;
	appa208_disp_data_t sdata;
} appa208_disp_t;

typedef struct __attribute__((__packed__))
{
	uint8_t start0;
	uint8_t start1;
	uint8_t cmd_code;
	uint8_t data_length;
	appa208_disp_t data;
	uint8_t checksum;
} appa208_rd_disp_ans_t;

typedef union
{
	uint8_t buffer[17];
	appa208_rd_disp_ans_t answer;
} appa208_rd_disp_ans_packet_t;


typedef enum {
	U_None = 0, U_V, U_mV, U_A, U_mA, U_dB, U_dBm, U_mF, U_uF, U_nF, U_GOhm, U_MOhm, U_kOhm, U_Ohm,
	U_percent, U_MHz, U_kHz, U_Hz, U_Cel, U_Far, U_sec, U_ms, U_us, U_ns, U_uA, U_min, U_kW, U_PF, U_F, U_W
} unit_t;

int appa208_open(const char *tty_filename, int *appa208_fd);
int appa208_close(int appa208_fd);

int appa208_read_info(int appa208_fd, appa208_info_t *info);
int appa208_read_disp(int appa208_fd, appa208_disp_t *disp);

double appa208_get_value(appa208_disp_data_t *disp_data);
unit_t appa208_get_unit(appa208_disp_data_t *disp_data);
int appa208_get_overload(appa208_disp_data_t *disp_data);

const char *appa208_str_ftcode(uint8_t ftcode);
const char *appa208_str_fcode(uint8_t fcode);
const char *appa208_str_word(uint16_t wcode);
const char *appa208_str_unit(unit_t unit);
const char *appa208_str_overload(uint8_t overload);
const char *appa208_str_data_content(uint8_t data_content);