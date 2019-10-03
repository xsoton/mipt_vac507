#include "main.h"

void *commander(void *arg)
{
	exp_t *p = (exp_t *)arg;
	char str[100];
	char *s;

	while(get_run(p))
	{
		printf("> ");

		s = fgets(str, 100, stdin);
		if (s == NULL)
		{
			fprintf(stderr, "# E: Exit\n");
			set_run(p, 0);
			break;
		}

		switch(str[0])
		{
			int ccount;
			case 'h':
				printf(
					"Help:\n"
					"\th -- this help;\n"
					"\tq -- exit the program;\n");
				break;
			case 'q':
				set_run(p, 0);
				break;
			default:
				ccount = strlen(str)-1;
				fprintf(stderr, "# E: Unknown commang (%.*s)\n", ccount, str);
				break;
		}
	}

	return NULL;
}
