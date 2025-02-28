int qj3003p_open(const char *tty_filename, int *qj3003p_fd);
int qj3003p_close(int qj3003p_fd);
int qj3003p_get_idn(int qj3003p_fd, char *str, int max_str_length);
int qj3003p_set_voltage(int qj3003p_fd, double voltage);
int qj3003p_get_voltage(int qj3003p_fd, double *voltage);
int qj3003p_set_current(int qj3003p_fd, double current);
int qj3003p_get_current(int qj3003p_fd, double *current);
int qj3003p_set_output(int qj3003p_fd, int output);
int qj3003p_get_status(int qj3003p_fd, int *output);