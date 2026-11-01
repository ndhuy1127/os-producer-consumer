#!/usr/bin/env python3
"""Repeat multi-thread runs of the real CLI; check every run with the full
stdout oracle of check_cli_log and print a result table (T07 and friends)."""
import argparse
import shlex
import time

import check_cli_log

STRESS = [
    ("T06a", (3, 3, 2, 100, 0, 0)),
    ("T06b", (2, 2, 5, 100, 0, 0)),             # C > N
    ("T07", (16, 4, 3, 10000, 0, 0)),           # 40000 DATA per run
    ("T09a", (1, 1, 4, 0, 0, 0)),               # K=0: only STOP, C > N
    ("T09b", (2, 2, 3, 30, 0, 10)),
    ("N1-8P8C", (1, 8, 8, 500, 0, 0)),          # one slot, 16 workers
    ("max-workers", (64, 64, 64, 100, 0, 0)),   # 64 producers and 64 consumers
]


def run_stress(runs=20, log=None):
    rows = []
    for name, config in STRESS:
        command = ["./bin/producer-consumer", *check_cli_log.arguments(config)]
        timeout = check_cli_log.time_budget(config)
        if log:
            log.write(f"\n$ {shlex.join(command)}\nrepeat={runs} timeout={timeout}s each; "
                      "full stdout oracle per run, log keeps summary lines only\n")
        samples = []
        for index in range(1, runs + 1):
            start = time.monotonic()
            run = check_cli_log.execute(command, timeout=timeout)
            elapsed = time.monotonic() - start
            assert not run.stderr
            result = check_cli_log.validate_runtime(run.stdout, config, False, name)
            samples.append((elapsed, result))
            if log:
                log.write(f"run={index} exit_code=0 elapsed_s={elapsed:.3f} oracle=PASS\n")
                log.write("".join(f"  {line}\n" for line in check_cli_log.summary_lines(run.stdout)))
                log.flush()
        times = [elapsed for elapsed, _ in samples]
        results = [result for _, result in samples]
        n, p, c, k, _, _ = config
        row = dict(
            name=name, config=config, runs=runs, passed=runs, data_per_run=p * k,
            elapsed_s=(round(min(times), 3), round(max(times), 3)),
            max_count=(min(r["max_count"] for r in results), max(r["max_count"] for r in results)),
            full_snapshots=(min(r["full_snapshots"] for r in results), max(r["full_snapshots"] for r in results)),
            wait_slot=(min(r["wait_slot"] for r in results), max(r["wait_slot"] for r in results)),
            wait_item=(min(r["wait_item"] for r in results), max(r["wait_item"] for r in results)),
            pending=(min(r["pending_after_producer_join"] for r in results),
                     max(r["pending_after_producer_join"] for r in results)),
            consumer_share=(min(min(r["consumer_data"]) for r in results),
                            max(max(r["consumer_data"]) for r in results)),
        )
        rows.append(row)
        print(f"PASS stress={name} runs={runs}/{runs} DATA/run={p * k} "
              f"time={row['elapsed_s'][0]}-{row['elapsed_s'][1]}s", flush=True)
    return rows


def span(pair):
    low, high = pair
    return str(low) if low == high else f"{low}–{high}"


def table(rows):
    lines = [
        "| Cấu hình | N/P/C/K/Dp/Dc | Lần chạy đạt | DATA mỗi lần | Thời gian (s) | count tối đa "
        "| Snapshot đầy | WAIT_SLOT | WAIT_ITEM | DATA còn đợi khi STOP | DATA ít/nhiều nhất của 1 consumer |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for r in rows:
        lines.append(
            f"| {r['name']} | {'/'.join(map(str, r['config']))} | {r['passed']}/{r['runs']} "
            f"| {r['data_per_run']} | {span(r['elapsed_s'])} | {span(r['max_count'])} "
            f"| {span(r['full_snapshots'])} | {span(r['wait_slot'])} | {span(r['wait_item'])} "
            f"| {span(r['pending'])} | {span(r['consumer_share'])} |")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runs", type=int, default=20, help="repetitions per configuration")
    rows = run_stress(parser.parse_args().runs)
    print(table(rows), end="")
    print(f"Stress PASS: {len(rows)} configurations, {sum(r['runs'] for r in rows)} runs")
