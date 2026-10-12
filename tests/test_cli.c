#include "cli.h"

#include <stdio.h>

/* Parser-only driver: accept large boundary values without pc_run/workers. */
int main(int argc, char **argv)
{
    struct cli_options options;
    int status = cli_parse(argc, argv, &options, stderr);
    if (status != PC_OK) return status;
    const struct pc_config *c = &options.config;
    printf("parsed: N=%zu P=%zu C=%zu K=%zu Dp_ms=%u Dc_ms=%u quiet=%d help=%d\n",
           c->capacity, c->producers, c->consumers, c->items_per_producer,
           c->producer_delay_ms, c->consumer_delay_ms, options.quiet, options.help);
    return PC_OK;
}
