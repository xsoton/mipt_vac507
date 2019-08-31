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
	uint8_t buffer[17];
	appa208_rd_info_ans_t answer;
} appa208_rd_info_ans_packet_t;


// display
typedef struct __attribute__((__packed__))
{
	uint8_t reading[3];
	uint8_t unit:5;
	uint8_t dot:3;
	uint8_t overload:1;
	uint8_t data_content:7;
} appa208_disp_data_t;

typedef struct __attribute__((__packed__))
{
	uint8_t auto_test:1;
	uint8_t fcode:7;
	uint8_t auto_range:1;
	uint8_t rcode:7;
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

const char *appa208_ftcodes[] = {"Manual Test", "Auto Test"};
const char *appa208_fcodes[] = {
	"None", "AC V", "DC V", "AC mV", "DC mV", "Ohm", "Continuity", "Diode", "Cap", "AC A",
	"DC A", "AC mA", "DC mA", "°C", "°F", "Frequency", "Duty", "Hz (V)", "Hz (mV)", "Hz (A)",
	"Hz (mA)", "AC+DC (V)", "AC+DC (mV)", "AC+DC (A)", "AC+DC (mA)", "LPF (V)", "LPF (mV)",
	"LPF (A)", "LPF (mA)", "AC uA", "DC uA", "DC A out", "DC A out (Slow Linear)",
	"DC A out (Fast Linear)", "DC A out (Slow Step)", "DC A out (Fast Step)", "LoopPower",
	"250ohmHart", "VoltSense", "PeakHold (V)", "PeakHold (mV)", "PeakHold (A)", "PeakHold (mA)",
	"LoZ AC V", "LoZ DC V", "LoZ AC+DC (V)", "LoZ LPF (V)", "LoZ Hz (V)", "LoZ PeakHold (V)",
	"Battery", "AC W", "DC W", "PF", "Flex AC A", "Flex LPF (A)", "Flex PeakHold (A)",
	"Flex Hz (A)", "Vharm", "Inrush", "Aharm", "Flex Inrush", "Flex Aharm", "PeakHold (uA)"};

const char *appa208_words[] = {
	"Space", "Full", "Beep", "APO", "b.Lit", "HAZ", "On", "Off", "Reset", "Start", "View",
	"Pause", "Fuse", "Probe", "dEF", "Clr", "Er", "Er1", "Er2", "Er3", "Dash", "Dash1",
	"Test", "Dash2", "bAtt", "diSLt", "noiSE", "FiLtr", "PASS", "null", "0-20", "4-20",
	"RATE", "SAVE", "LOAd", "YES", "SEnd", "Ahold", "Auto", "Cntin", "CAL", "Version",
	"OL (not use)", "FULL", "HALF", "Lo", "Hi", "digit", "rdy", "dISC", "outF", "OLA",
	"OLV", "OLVA", "bAd", "TEMP"};

const char *appa208_units[] = {
	"None", "V", "mV", "A", "mA", "dB", "dBm", "mF", "uF", "nF", "GΩ", "MΩ", "kΩ", "Ω", "%%",
	"MHz", "kHz", "Hz", "°C", "°F", "sec", "ms", "us", "ns", "uA", "min", "kW", "PF", "F", "W"};

const char *appa208_ols[] = {"Not Overload", "Overload"};

const char *appa208_dcs[] = {
	"Measuring data", "Frequency", "Cycle", "Duty", "Memory Stamp", "Memory Save", "Memory Load",
	"Log Save", "Log Load", "Log Rate", "REL Δ", "REL %", "REL Reference", "Maximum", "Minimum",
	"Average", "Peak Hold Max", "Peak Hold Min", "dBm", "dB", "Auto Hold", "Setup", "Log Stamp",
	"Log Max", "Log Min", "Log TP", "Hold", "Current Output", "CurOut 0-20mA %%", "CurOut 4-20mA %%"};