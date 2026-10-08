#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { WORKER_COUNT = 3 };

struct worker_result {
    unsigned long long sum;
};

struct worker_args {
    unsigned int id;
    unsigned int limit;
    struct worker_result *result;
};

static void *worker(void *argument)
{
    const struct worker_args *args = argument;
    unsigned long long sum = 0;

    for (unsigned int value = 1; value <= args->limit; ++value) {
        sum += value;
    }

    /* Moi worker chi ghi vao o ket qua cua minh. */
    args->result->sum = sum;
    printf("worker %u: sum(1..%u) = %llu\n", args->id, args->limit, sum);
    return NULL;
}

int main(void)
{
    pthread_t threads[WORKER_COUNT];
    struct worker_result results[WORKER_COUNT] = {0};
    struct worker_args args[WORKER_COUNT];
    size_t created = 0;
    size_t verified = 0;
    int status = EXIT_SUCCESS;
    int join_failed = 0;

    /* Mang cua main song den khi join xong, khong truyen dia chi bien i. */
    for (size_t i = 0; i < WORKER_COUNT; ++i) {
        args[i].id = (unsigned int)i + 1;
        args[i].limit = ((unsigned int)i + 1) * 10;
        args[i].result = &results[i];

        int error = pthread_create(&threads[i], NULL, worker, &args[i]);
        if (error != 0) {
            fprintf(stderr, "pthread_create worker %u: %s\n",
                    args[i].id, strerror(error));
            status = EXIT_FAILURE;
            break;
        }
        ++created;
    }

    /* Ke ca create loi giua chung, van join cac luong da tao thanh cong. */
    for (size_t i = 0; i < created; ++i) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join worker %u: %s\n",
                    args[i].id, strerror(error));
            status = EXIT_FAILURE;
            join_failed = 1;
            continue;
        }

        /* Chi doc ket qua sau join; dung cong thuc de doi chieu vong lap. */
        unsigned long long limit = args[i].limit;
        unsigned long long expected = limit * (limit + 1) / 2;
        if (results[i].sum != expected) {
            fprintf(stderr, "worker %u: expected %llu, got %llu\n",
                    args[i].id, expected, results[i].sum);
            status = EXIT_FAILURE;
            continue;
        }

        ++verified;
        printf("joined worker %u: result = %llu, expected = %llu, OK\n",
               args[i].id, results[i].sum, expected);
    }

    if (status != EXIT_SUCCESS || created != WORKER_COUNT ||
        verified != WORKER_COUNT) {
        fprintf(stderr, "incomplete: created %zu/%d, verified %zu/%d\n",
                created, WORKER_COUNT, verified, WORKER_COUNT);
        if (join_failed) {
            /* Chua biet luong loi join da dung; exit giu stack den khi process dung. */
            exit(EXIT_FAILURE);
        }
        return EXIT_FAILURE;
    }

    printf("completed: all %d workers joined and verified\n", WORKER_COUNT);
    return EXIT_SUCCESS;
}
