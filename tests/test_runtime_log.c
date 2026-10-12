#include "runtime_log.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int check(int quiet, int status, int verified)
{
    struct pc_record records[] = {
        {UINT64_MAX, PC_ENQUEUE, PC_PRODUCER, {ITEM_DATA, UINT64_MAX, -7},
         UINT64_MAX, 1, 2, PC_WAIT_SLOT},
        {2, PC_DEQUEUE, PC_CONSUMER, {ITEM_STOP, UINT64_MAX, 0},
         3, 0, 2, PC_WAIT_ITEM}
    }; /* Formatting fixture only; not runtime/FIFO evidence. */
    struct pc_result result = {
        .config = {2, 1, 1, 1, 0, 0}, .summary = {.verified = verified},
        .records = records, .record_count = 2
    };
    FILE *output = tmpfile();
    if (output == NULL) return 0;
    int ok = runtime_log_result(output, &result, status, quiet) == PC_OK;
    char text[4096] = {0};
    if (fseek(output, 0, SEEK_SET) != 0) ok = 0;
    size_t length = fread(text, 1, sizeof(text) - 1, output);
    if (ferror(output) || length == sizeof(text) - 1) ok = 0;
    if (fclose(output) != 0) ok = 0;
    const char *conclusion = status == PC_OK && verified ? "verification=PASS" : "verification=FAIL";
    ok = ok && strstr(text, conclusion) != NULL && strstr(text, "records=2") != NULL;
    if (quiet) {
        ok = ok && strstr(text, "op_seq=") == NULL;
    } else {
        ok = ok && strstr(text, "op_seq=18446744073709551615") != NULL &&
             strstr(text, "actor=P0 kind=DATA data_id=18446744073709551615 value=-7") != NULL &&
             strstr(text, "actor=C0 kind=STOP data_id=- value=0") != NULL &&
             strstr(text, "wait_reason=WAIT_SLOT") != NULL &&
             strstr(text, "wait_reason=WAIT_ITEM") != NULL;
    }
    return ok;
}

int main(void)
{
    size_t passed = 0, total = 0;
    for (int quiet = 0; quiet <= 1; ++quiet) {
        const int statuses[] = {PC_OK, PC_RUNTIME_ERROR, PC_OK};
        const int verified[] = {1, 1, 0};
        for (size_t i = 0; i < 3; ++i) {
            int ok = check(quiet, statuses[i], verified[i]);
            ++total; passed += ok;
            printf("%s log quiet=%d status=%d verified=%d\n",
                   ok ? "PASS" : "FAIL", quiet, statuses[i], verified[i]);
        }
    }
    /* Read-only stream rejects output at the OS level; cannot modify source. */
    FILE *read_only = fopen("tests/test_runtime_log.c", "r");
    struct pc_result result = {.config = {8, 1, 1, 0, 0, 0}};
    int ok = read_only != NULL &&
             runtime_log_result(read_only, &result, PC_OK, 0) == PC_RUNTIME_ERROR;
    if (read_only != NULL && fclose(read_only) != 0) ok = 0;
    ++total; passed += ok;
    printf("%s log output error propagation\n", ok ? "PASS" : "FAIL");
    printf("Log formatter tests: %zu/%zu %s\n", passed, total, passed == total ? "PASS" : "FAIL");
    return passed == total ? PC_OK : PC_RUNTIME_ERROR;
}
