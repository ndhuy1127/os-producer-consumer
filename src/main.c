#include "pc.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1) {
        fprintf(stderr, "Week 2: CLI parser pending; run without arguments (N=8 P=1 C=1 K=20 Dp_ms=0 Dc_ms=0).\n");
        return PC_CONFIG_ERROR;
    }
    struct pc_config config = pc_default_config();
    struct pc_result result = {0};
    int status = pc_run(&config, &result);
    const struct pc_summary *s = &result.summary;
    printf("config: N=%zu P=%zu C=%zu K=%zu Dp_ms=%u Dc_ms=%u\n",
           config.capacity, config.producers, config.consumers,
           config.items_per_producer, config.producer_delay_ms, config.consumer_delay_ms);
    printf("DATA: enqueued=%zu dequeued=%zu processed=%zu missing=%zu duplicate=%zu out_of_range=%zu value_errors=%zu FIFO_errors=%zu\n",
           s->data_enqueued, s->data_dequeued, s->data_processed, s->missing,
           s->duplicate, s->out_of_range, s->value_errors, s->fifo_errors);
    printf("buffer: min=%zu max=%zu final=%zu count_errors=%zu; STOP: enqueued=%zu dequeued=%zu\n",
           s->min_count, s->max_count, s->final_count, s->count_errors,
           s->stop_enqueued, s->stop_dequeued);
    printf("workers: created=%zuP/%zuC joined=%zuP/%zuC; verification=%s run_status=%d\n",
           s->producers_created, s->consumers_created, s->producers_joined,
           s->consumers_joined, s->verified ? "PASS" : "FAIL", status);
    if (status != PC_OK) fprintf(stderr, "producer-consumer: run failed (status=%d)\n", status);
    pc_result_destroy(&result);
    return status;
}
