#include <stdint.h>
#include <inttypes.h>

int appa208_open(const char *tty_filename, int *appa208_fd);
int appa208_close(int appa208_fd);
int appa208_read_info(int appa208_fd, char *model, char *serial, uint16_t *model_id, uint16_t *fw_version);
