#define _POSIX_C_SOURCE 200809L
#include "cli.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

enum option_id { CAPACITY, PRODUCERS, CONSUMERS, ITEMS, PRODUCER_DELAY, CONSUMER_DELAY, QUIET };

struct numeric_option {
    const char *name;
    enum option_id id;
    uintmax_t minimum, maximum;
};

static const struct numeric_option numeric_options[] = {
    {"--capacity", CAPACITY, 1, PC_MAX_CAPACITY},
    {"--producers", PRODUCERS, 1, PC_MAX_WORKERS},
    {"--consumers", CONSUMERS, 1, PC_MAX_WORKERS},
    {"--items-per-producer", ITEMS, 0, PC_MAX_TOTAL_ITEMS},
    {"--producer-delay-ms", PRODUCER_DELAY, 0, PC_MAX_DELAY_MS},
    {"--consumer-delay-ms", CONSUMER_DELAY, 0, PC_MAX_DELAY_MS}
};

static int number(const struct numeric_option *option, const char *text,
                  uintmax_t *value, FILE *errors)
{
    if (*text == '\0') {
        fprintf(errors, "CLI: %s requires a nonempty unsigned decimal value\n", option->name);
        return PC_CONFIG_ERROR;
    }
    for (const char *p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            fprintf(errors, "CLI: %s requires digits 0..9 only (no sign, whitespace or suffix)\n", option->name);
            return PC_CONFIG_ERROR;
        }
    }
    errno = 0;
    char *end;
    uintmax_t parsed = strtoumax(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0') {
        fprintf(errors, "CLI: %s value overflows unsigned integer conversion\n", option->name);
        return PC_CONFIG_ERROR;
    }
    if (parsed < option->minimum || parsed > option->maximum) {
        fprintf(errors, "CLI: %s must be in [%" PRIuMAX ",%" PRIuMAX "]\n",
                option->name, option->minimum, option->maximum);
        return PC_CONFIG_ERROR;
    }
    uintmax_t target_max = option->id == PRODUCER_DELAY || option->id == CONSUMER_DELAY
                          ? UINT_MAX : SIZE_MAX;
    if (parsed > target_max) {
        fprintf(errors, "CLI: %s exceeds the destination type\n", option->name);
        return PC_CONFIG_ERROR;
    }
    *value = parsed;
    return PC_OK;
}

static int resources_valid(const struct pc_config *config, FILE *errors)
{
    /* Match the core's capacity/array limits; the core remains the final check
     * before allocation. Each value has already been range-checked. */
    size_t n = config->capacity, k = config->items_per_producer;
    if (n > SEM_VALUE_MAX || n > UINT_MAX || n > SIZE_MAX / sizeof(struct item) ||
        n > SIZE_MAX / sizeof(uint64_t)) {
        fprintf(errors, "CLI: --capacity exceeds semaphore or allocation limits\n");
        return PC_CONFIG_ERROR;
    }
    if (k != 0 && config->producers > PC_MAX_TOTAL_ITEMS / k) {
        fprintf(errors, "CLI: total DATA P*K = --producers * --items-per-producer must be <= %d\n",
                PC_MAX_TOTAL_ITEMS);
        return PC_CONFIG_ERROR;
    }
    size_t total = config->producers * k;
    if (total > UINT64_MAX || total > SIZE_MAX / sizeof(unsigned int) ||
        config->consumers > SIZE_MAX - total) {
        fprintf(errors, "CLI: DATA/STOP or ID/allocation size overflows\n");
        return PC_CONFIG_ERROR;
    }
    size_t entries = total + config->consumers;
    if (entries > SIZE_MAX / 2 || entries > UINT64_MAX / 2 ||
        2 * entries > SIZE_MAX / sizeof(struct pc_record)) {
        fprintf(errors, "CLI: operation record size overflows\n");
        return PC_CONFIG_ERROR;
    }
    return PC_OK;
}

int cli_parse(int argc, char *const argv[], struct cli_options *options, FILE *errors)
{
    if (errors == NULL) errors = stderr;
    if (argc < 1 || argv == NULL || options == NULL) {
        fprintf(errors, "CLI: invalid parser arguments\n");
        return PC_CONFIG_ERROR;
    }
    struct cli_options parsed = {.config = pc_default_config()};
    unsigned int seen = 0;
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == NULL) {
            fprintf(errors, "CLI: null argument\n");
            return PC_CONFIG_ERROR;
        }
        if (strcmp(argv[i], "--help") == 0) {
            if (argc != 2) {
                fprintf(errors, "CLI: --help must be used alone\n");
                return PC_CONFIG_ERROR;
            }
            parsed.help = 1;
            *options = parsed;
            return PC_OK;
        }
        if (strcmp(argv[i], "--quiet") == 0) {
            if (seen & (1U << QUIET)) {
                fprintf(errors, "CLI: duplicate option --quiet\n");
                return PC_CONFIG_ERROR;
            }
            seen |= 1U << QUIET;
            parsed.quiet = 1;
            continue;
        }
        const struct numeric_option *option = NULL;
        for (size_t j = 0; j < sizeof(numeric_options) / sizeof(numeric_options[0]); ++j) {
            if (strcmp(argv[i], numeric_options[j].name) == 0) {
                option = &numeric_options[j];
                break;
            }
        }
        if (option == NULL) {
            fprintf(errors, "CLI: unknown option or positional argument '%s'\n", argv[i]);
            return PC_CONFIG_ERROR;
        }
        if (seen & (1U << option->id)) {
            fprintf(errors, "CLI: duplicate option %s\n", option->name);
            return PC_CONFIG_ERROR;
        }
        seen |= 1U << option->id;
        if (i + 1 >= argc || argv[i + 1] == NULL || strncmp(argv[i + 1], "--", 2) == 0) {
            fprintf(errors, "CLI: missing value for %s\n", option->name);
            return PC_CONFIG_ERROR;
        }
        uintmax_t value;
        if (number(option, argv[++i], &value, errors) != PC_OK) return PC_CONFIG_ERROR;
        switch (option->id) {
        case CAPACITY: parsed.config.capacity = (size_t)value; break;
        case PRODUCERS: parsed.config.producers = (size_t)value; break;
        case CONSUMERS: parsed.config.consumers = (size_t)value; break;
        case ITEMS: parsed.config.items_per_producer = (size_t)value; break;
        case PRODUCER_DELAY: parsed.config.producer_delay_ms = (unsigned int)value; break;
        case CONSUMER_DELAY: parsed.config.consumer_delay_ms = (unsigned int)value; break;
        case QUIET: break; /* Numeric table never uses this entry. */
        }
    }
    if (resources_valid(&parsed.config, errors) != PC_OK) return PC_CONFIG_ERROR;
    *options = parsed;
    return PC_OK;
}

int cli_print_help(FILE *output, const char *program)
{
    if (fprintf(output,
        "Usage: %s [options]\n"
        "Finite Producer-Consumer: P producers, C consumers, bounded FIFO buffer.\n"
        "  --capacity N                1..65536 (also semaphore/size limits), default 8\n"
        "  --producers P               1..64, default 1\n"
        "  --consumers C               1..64, default 1\n"
        "  --items-per-producer K      0..1000000 and P*K <= 1000000, default 20; K=0 is valid\n"
        "Producer p creates DATA IDs p*K..p*K+K-1; main sends one STOP per consumer.\n"
        "  --producer-delay-ms Dp      0..60000 milliseconds, default 0\n"
        "  --consumer-delay-ms Dc      0..60000 milliseconds, default 0\n"
        "  --quiet                     suppress operation records; keep verification/summary\n"
        "  --help                      use alone; no workers\n"
        "Use --option value only; --option=value is not supported.\n"
        "Values: unsigned decimal digits only. No duplicate/unknown options.\n"
        "Records reflect operations inside guard and are printed after all joins.\n"
        "WAIT_SLOT/WAIT_ITEM mean EAGAIN observed, not measured kernel blocking.\n"
        "Exit: 0 success/help, 1 runtime/verification/output error, 2 argument/config error.\n",
        program) < 0 || fflush(output) == EOF) {
        return PC_RUNTIME_ERROR;
    }
    return PC_OK;
}
