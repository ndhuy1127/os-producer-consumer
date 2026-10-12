#ifndef OS_PC_RUNTIME_LOG_H
#define OS_PC_RUNTIME_LOG_H

#include <stdio.h>
#include "pc.h"

/* Read result only after pc_run returns. Borrow the snapshot, never own/free it.
 * Quiet suppresses records, never the oracle/summary. Return 1 on output error.
 * PASS requires BOTH run_status==PC_OK and summary.verified.
 */
int runtime_log_result(FILE *output, const struct pc_result *result, int run_status, int quiet);

#endif
