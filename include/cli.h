#ifndef OS_PC_CLI_H
#define OS_PC_CLI_H

#include <stdio.h>
#include "pc.h"

struct cli_options {
    struct pc_config config;
    int quiet;
    int help;
};

/* No allocation/workers. Accept only --option value, and standalone --help.
 * On error, report to errors (stderr if NULL), return PC_CONFIG_ERROR, and
 * leave *options unchanged. Defaults come from the existing core.
 */
int cli_parse(int argc, char *const argv[], struct cli_options *options, FILE *errors);
int cli_print_help(FILE *output, const char *program);

#endif
