#include "pc.h"
#include "cli.h"
#include "runtime_log.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct cli_options options;
    int status = cli_parse(argc, argv, &options, stderr);
    if (status != PC_OK) return status;
    if (options.help) {
        status = cli_print_help(stdout, argv[0]);
        if (status != PC_OK) fprintf(stderr, "producer-consumer: help output failed\n");
        return status;
    }
    struct pc_result result = {0};
    status = pc_run(&options.config, &result);
    if (status == PC_OK && !result.summary.verified) status = PC_RUNTIME_ERROR;
    if (runtime_log_result(stdout, &result, status, options.quiet) != PC_OK) {
        int error = errno;
        fprintf(stderr, "producer-consumer: log output failed: %s\n", strerror(error));
        status = PC_RUNTIME_ERROR;
    }
    pc_result_destroy(&result);
    if (status != PC_OK) fprintf(stderr, "producer-consumer: run failed (status=%d)\n", status);
    return status;
}
