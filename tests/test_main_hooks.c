#define _POSIX_C_SOURCE 200809L
#include "pc.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int __real_pc_run(const struct pc_config *, struct pc_result *);
void __real_pc_result_destroy(struct pc_result *);
int __real_pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
int __real_sem_init(sem_t *, int, unsigned int);
int __real_sem_destroy(sem_t *);
static int init_calls, destroy_calls;

static int mode(const char *expected)
{
    const char *selected = getenv("PC_MAIN_TEST_MODE");
    return selected != NULL && strcmp(selected, expected) == 0;
}

int __wrap_pc_run(const struct pc_config *config, struct pc_result *result)
{
    if (mode("forbid")) {
        fprintf(stderr, "FORBIDDEN pc_run: parser/help did not stop before core\n");
        _Exit(99);
    }
    int status = __real_pc_run(config, result);
    if (mode("unverified") && status == PC_OK) result->summary.verified = 0;
    fprintf(stderr, "test_hook: pc_run status=%d verified=%d records_owned=%d\n",
            status, result->summary.verified, result->records != NULL);
    return status;
}

int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start)(void *), void *argument)
{
    if (mode("forbid")) {
        fprintf(stderr, "FORBIDDEN pthread_create: parser/help started a worker\n");
        _Exit(99);
    }
    return __real_pthread_create(thread, attr, start, argument);
}

int __wrap_sem_init(sem_t *semaphore, int shared, unsigned int value)
{
    ++init_calls;
    if (mode("init-error") && init_calls == 1) {
        errno = ENOSPC;
        return -1;
    }
    return __real_sem_init(semaphore, shared, value);
}

int __wrap_sem_destroy(sem_t *semaphore)
{
    ++destroy_calls;
    int status = __real_sem_destroy(semaphore);
    if (status == 0 && mode("cleanup-error") && destroy_calls == 1) {
        errno = EINVAL; /* Simulated error after releasing the test semaphore. */
        return -1;
    }
    return status;
}

void __wrap_pc_result_destroy(struct pc_result *result)
{
    int owned = result->records != NULL;
    __real_pc_result_destroy(result);
    fprintf(stderr, "test_hook: result_destroy records_before=%d records_after=%d\n",
            owned, result->records != NULL);
}
