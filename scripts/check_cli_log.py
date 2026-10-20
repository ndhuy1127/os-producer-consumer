#!/usr/bin/env python3
"""Test the real CLI/log, with a linear FIFO oracle and isolated main hooks."""
from collections import Counter
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PRESETS = [
    ("default", (8, 1, 1, 20, 0, 0)), ("source-N5", (5, 1, 1, 20, 0, 0)),
    ("T02", (8, 1, 1, 100, 0, 0)), ("T03", (1, 1, 1, 100, 0, 0)),
    ("T04", (2, 1, 1, 40, 0, 20)), ("T05", (2, 1, 1, 40, 20, 0)),
    ("K0", (1, 1, 1, 0, 0, 0)), ("queued-stop", (8, 1, 1, 20, 0, 20)),
    # Week 3: several producers/consumers (T07 repeats in check_stress.py).
    ("T06a", (3, 3, 2, 100, 0, 0)), ("T06b", (2, 2, 5, 100, 0, 0)),
    ("T09a", (1, 1, 4, 0, 0, 0)), ("T09b", (2, 2, 3, 30, 0, 10)),
    ("T04-multi", (2, 3, 2, 40, 0, 20)), ("T05-multi", (2, 2, 3, 40, 20, 0)),
    ("queued-stop-multi", (8, 2, 3, 40, 0, 20)),
]
NEED_FULL = {"T04", "T04-multi"}
NEED_EMPTY = {"T05", "T05-multi"}
NEED_PENDING = {"queued-stop", "queued-stop-multi"}
FLAGS = ["--capacity", "--producers", "--consumers", "--items-per-producer",
         "--producer-delay-ms", "--consumer-delay-ms"]
RECORD = re.compile(
    r"op_seq=(\d+) op=(ENQUEUE|DEQUEUE) actor=(P\d+|C\d+|MAIN) kind=(DATA|STOP) "
    r"data_id=(\d+|-) value=(-?\d+) enqueue_seq=(\d+) count_after=(\d+) "
    r"capacity=(\d+) wait_reason=(NONE|WAIT_SLOT|WAIT_ITEM)"
)
SUMMARY_LINES = 7  # config, DATA, buffer, waits, producers, consumers, workers


def arguments(config):
    return [token for flag, value in zip(FLAGS, config) for token in (flag, str(value))]


def time_budget(config):
    """Conservative deadline: every delay serialized, doubled, plus 5 s."""
    _, p, _, k, dp, dc = config
    return 5 + (2 * p * k * (dp + dc) + 999) // 1000


def execute(command, expected=0, timeout=5, log=None, mode=None):
    env = os.environ.copy()
    if mode is not None:
        env["PC_MAIN_TEST_MODE"] = mode
    if log:
        log.write(f"\n$ {shlex.join(command)}\ntimeout={timeout}s expected_exit={expected}\n")
        if mode is not None:
            log.write(f"PC_MAIN_TEST_MODE={mode} (test binary only)\n")
        log.flush()
    run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                         timeout=timeout, env=env)
    if log:
        log.write("stdout:\n" + run.stdout + "stderr:\n" + run.stderr +
                  f"exit_code={run.returncode}\n")
        log.flush()
    assert run.returncode == expected, (command, run.returncode, expected, run.stderr)
    return run


def matched(lines, pattern):
    found = [re.fullmatch(pattern, line) for line in lines]
    found = [m for m in found if m is not None]
    assert len(found) == 1, (pattern, lines[-SUMMARY_LINES:])
    return found[0].groups()


def summary_lines(stdout):
    """Non-record lines only, for compact logs of very long runs."""
    return [line for line in stdout.splitlines() if not line.startswith("op_seq=")]


def validate_runtime(stdout, config, quiet, name):
    n, p, c, k, dp, dc = config
    total = p * k
    lines = stdout.splitlines()
    actual_config = matched(lines, r"config: N=(\d+) P=(\d+) C=(\d+) K=(\d+) Dp_ms=(\d+) Dc_ms=(\d+)")
    assert tuple(map(int, actual_config)) == config
    data = tuple(map(int, matched(lines, r"DATA: enqueued=(\d+) dequeued=(\d+) processed=(\d+) missing=(\d+) duplicate=(\d+) out_of_range=(\d+) value_errors=(\d+) FIFO_errors=(\d+) order_errors=(\d+)")))
    buffer = tuple(map(int, matched(lines, r"buffer: min=(\d+) max=(\d+) final=(\d+) count_errors=(\d+); STOP: enqueued=(\d+) dequeued=(\d+)")))
    waits = tuple(map(int, matched(lines, r"waits: WAIT_SLOT=(\d+) WAIT_ITEM=(\d+) records=(\d+) data_pending_after_producer_join=(\d+)")))
    producers = re.findall(r" P(\d+)=(\d+)", matched(lines, r"producers\(DATA\):((?: P\d+=\d+)*)")[0])
    consumers = re.findall(r" C(\d+)=(\d+)/(\d+)", matched(lines, r"consumers\(DATA/STOP\):((?: C\d+=\d+/\d+)*)")[0])
    workers = matched(lines, r"workers: created=(\d+)P/(\d+)C joined=(\d+)P/(\d+)C; verification=(PASS|FAIL) run_status=(\d+)")
    assert data == (total, total, total, 0, 0, 0, 0, 0, 0)
    assert buffer[0] == 0 and 0 <= buffer[1] <= n and buffer[2:] == (0, 0, c, c)
    assert tuple(map(int, workers[:4])) == (p, c, p, c) and workers[4:] == ("PASS", "0")
    assert waits[2] == 2 * (total + c) and 0 <= waits[3] <= n
    # Per-worker summary: every producer made K DATA; every consumer took one STOP.
    assert [(int(i), int(v)) for i, v in producers] == [(i, k) for i in range(p)]
    assert [int(i) for i, _, _ in consumers] == list(range(c))
    assert all(int(stop) == 1 for _, _, stop in consumers)
    consumer_data = [int(value) for _, value, _ in consumers]
    assert sum(consumer_data) == total
    record_lines = [line for line in lines if line.startswith("op_seq=")]
    assert len(lines) == len(record_lines) + SUMMARY_LINES  # no unknown/incomplete lines
    result = dict(name=name, quiet=quiet, config=config, exit_code=0, outcome="PASS",
                  records_retained=waits[2], wait_slot=waits[0], wait_item=waits[1],
                  pending_after_producer_join=waits[3], max_count=buffer[1],
                  consumer_data=consumer_data, full_snapshots=None, empty_snapshots=None)
    if quiet:
        assert not record_lines
        if name in NEED_FULL: assert buffer[1] == n and waits[0] > 0
        if name in NEED_EMPTY: assert waits[1] > 0
        if name in NEED_PENDING: assert waits[3] > 0
        return result
    assert len(record_lines) == 2 * (total + c)
    queue, taken = [], 0  # independent append-only queue, no circular indexes
    enqueued, dequeued = Counter(), Counter()
    next_in, next_out = [0] * p, [0] * p
    taken_by, stops_by = [0] * c, [0] * c
    stops_in = stops_out = full = empty = slots = items = producer_slots = 0
    max_count = 0
    for sequence, line in enumerate(record_lines, 1):
        match = RECORD.fullmatch(line)
        assert match is not None, line
        op_seq, op, actor, kind, data_id, value, enqueue_seq, count, capacity, wait = match.groups()
        op_seq, value, enqueue_seq, count, capacity = map(int, (op_seq, value, enqueue_seq, count, capacity))
        assert op_seq == sequence and capacity == n
        item = (kind, int(data_id) if data_id != "-" else None, value)
        if kind == "DATA":
            assert item[1] is not None and 0 <= item[1] < total and value == item[1] % 1000 + 1
            owner, j = divmod(item[1], k)  # ID p*K+j belongs to producer p
        else:
            assert data_id == "-" and value == 0
        if op == "ENQUEUE":
            assert wait in ("NONE", "WAIT_SLOT") and enqueue_seq == len(queue) + 1
            slots += wait == "WAIT_SLOT"
            queue.append((item, enqueue_seq))
            if kind == "DATA":
                assert stops_in == 0 and actor == f"P{owner}" and j == next_in[owner]
                next_in[owner] += 1
                producer_slots += wait == "WAIT_SLOT"
                enqueued[item[1]] += 1
            else:
                assert actor == "MAIN" and sum(enqueued.values()) == total
                stops_in += 1
        else:
            consumer = re.fullmatch(r"C(\d+)", actor)
            assert consumer is not None and int(consumer.group(1)) < c
            index = int(consumer.group(1))
            assert wait in ("NONE", "WAIT_ITEM") and taken < len(queue)
            assert queue[taken] == (item, enqueue_seq)
            taken += 1
            items += wait == "WAIT_ITEM"
            if kind == "DATA":
                assert stops_out == 0 and j == next_out[owner]
                next_out[owner] += 1
                dequeued[item[1]] += 1
                taken_by[index] += 1
            else:
                assert sum(dequeued.values()) == total
                stops_out += 1
                stops_by[index] += 1
        assert count == len(queue) - taken and 0 <= count <= n
        full += count == n
        empty += count == 0
        max_count = max(max_count, count)
    expected = Counter(range(total))
    assert enqueued == dequeued == expected and taken == len(queue) and stops_in == stops_out == c
    assert next_in == next_out == [k] * p and stops_by == [1] * c and taken_by == consumer_data
    assert (slots, items) == waits[:2] and max_count == buffer[1]
    if name in NEED_FULL: assert full > 0 and producer_slots > 0
    if name in NEED_EMPTY: assert empty > 0 and items > 0
    if name in NEED_PENDING: assert waits[3] > 0
    result.update(full_snapshots=full, empty_snapshots=empty, producer_wait_slot=producer_slots)
    return result


def invalid_cases():
    cases = [
        ("unknown", ["--unknown"], "unknown"),
        ("positional", ["3"], "positional"),
        ("equals-not-supported", ["--capacity=5"], "unknown"),
        ("end-options-not-supported", ["--"], "unknown"),
        ("quiet-duplicate", ["--quiet", "--quiet"], "duplicate"),
        ("quiet-value", ["--quiet", "1"], "positional"),
        ("help-quiet", ["--help", "--quiet"], "alone"),
        ("quiet-help", ["--quiet", "--help"], "alone"),
        ("help-value", ["--help", "--capacity", "2"], "alone"),
        ("help-duplicate", ["--help", "--help"], "alone"),
        ("flag-as-value", ["--capacity", "--quiet"], "missing"),
    ]
    bad_values = ["-1", "+1", "1x", "1.5", "", " 1", "1 ", "1\t", "1\n",
                  "١", "0x1", "1e1", "9" * 100]
    defaults = [8, 1, 1, 20, 0, 0]
    for flag, valid in zip(FLAGS, defaults):
        cases.append((flag + "-missing", [flag], "missing"))
        cases.append((flag + "-duplicate", [flag, str(valid), flag, str(valid)], "duplicate"))
        for index, bad in enumerate(bad_values):
            cases.append((flag + f"-bad-{index}", [flag, bad], flag))
    for flag in FLAGS[:3]:
        cases.append((flag + "-zero", [flag, "0"], flag))
    for flag, value in [("--capacity", 65537), ("--producers", 65), ("--consumers", 65),
                        ("--items-per-producer", 1000001),
                        ("--producer-delay-ms", 60001), ("--consumer-delay-ms", 60001)]:
        cases.append((flag + "-above", [flag, str(value)], flag))
    for flag in FLAGS[1:3]:
        cases.append((flag + "-uintmax", [flag, "18446744073709551615"], flag))
    # Each value is in range, but P*K exceeds the 1000000 DATA limit.
    for p, k in [(2, 600000), (64, 15626)]:
        cases.append((f"total-P{p}-K{k}", ["--producers", str(p), "--items-per-producer", str(k)], "P*K"))
    return cases


def run_suite(logs=None):
    logs = logs or {}
    rows = []
    for name, config in PRESETS:
        for quiet in (False, True):
            args = [] if name == "default" else arguments(config)
            if quiet: args.append("--quiet")
            run = execute(["./bin/producer-consumer", *args], timeout=time_budget(config),
                          log=logs.get("runtime"))
            assert not run.stderr
            row = validate_runtime(run.stdout, config, quiet, name)
            rows.append(row)
            print(f"PASS runtime={name} quiet={int(quiet)} exit=0", flush=True)
    cases = invalid_cases()
    for label, args, expected_error in cases:
        for binary, mode in [("./bin/producer-consumer", None), ("./bin/main-hook-test", "forbid")]:
            run = execute([binary, *args], expected=2, log=logs.get("cli"), mode=mode)
            assert not run.stdout and "CLI:" in run.stderr and expected_error in run.stderr
            assert "FORBIDDEN" not in run.stderr
        print(f"PASS invalid={label} production_exit=2 hook_exit=2 no_core_or_workers", flush=True)
    for binary, mode in [("./bin/producer-consumer", None), ("./bin/main-hook-test", "forbid")]:
        run = execute([binary, "--help"], log=logs.get("cli"), mode=mode)
        assert not run.stderr and run.stdout.startswith("Usage:") and all(flag in run.stdout for flag in FLAGS)
        assert "1..64" in run.stdout and "P*K <= 1000000" in run.stdout
        assert "verification=" not in run.stdout
    # Positive control: prove the forbidden-core hook is active in this binary.
    run = execute(["./bin/main-hook-test", "--items-per-producer", "0"], expected=99,
                  log=logs.get("cli"), mode="forbid")
    assert "FORBIDDEN pc_run" in run.stderr
    boundaries = [
        ("defaults", [], (8, 1, 1, 20, 0, 0), 0, 0),
        ("lower", arguments((1, 1, 1, 0, 0, 0)), (1, 1, 1, 0, 0, 0), 0, 0),
        ("upper", arguments((65536, 1, 1, 1000000, 60000, 60000)) + ["--quiet"],
         (65536, 1, 1, 1000000, 60000, 60000), 1, 0),
        ("upper-workers", arguments((65536, 64, 64, 15625, 60000, 60000)),
         (65536, 64, 64, 15625, 60000, 60000), 0, 0),  # P*K = 1000000 exactly
        ("leading-zeros", ["--capacity", "00001", "--items-per-producer", "00000"],
         (1, 1, 1, 0, 0, 0), 0, 0),
        ("help", ["--help"], (8, 1, 1, 20, 0, 0), 0, 1),
    ]
    for label, args, config, quiet, help_flag in boundaries:
        run = execute(["./bin/cli-parse-test", *args], log=logs.get("cli"))
        assert not run.stderr
        values = matched(run.stdout.splitlines(), r"parsed: N=(\d+) P=(\d+) C=(\d+) K=(\d+) Dp_ms=(\d+) Dc_ms=(\d+) quiet=(\d+) help=(\d+)")
        assert tuple(map(int, values)) == (*config, quiet, help_flag)
        print(f"PASS parser-only={label} exit=0 no_runtime_claim", flush=True)
    run = execute(["./bin/log-test"], log=logs.get("log"))
    assert not run.stderr and "Log formatter tests: 8/8 PASS" in run.stdout
    for mode in ("init-error", "cleanup-error", "unverified"):
        for quiet in (False, True):
            args = ["--capacity", "1", "--producers", "2", "--consumers", "2",
                    "--items-per-producer", "3"]
            if quiet: args.append("--quiet")
            run = execute(["./bin/main-hook-test", *args], expected=1, log=logs.get("log"), mode=mode)
            assert "verification=FAIL run_status=1" in run.stdout and "verification=PASS" not in run.stdout
            assert "result_destroy records_before=1 records_after=0" in run.stderr
            if mode == "cleanup-error": assert "pc_run status=1 verified=1" in run.stderr
            if quiet: assert "op_seq=" not in run.stdout
            print(f"PASS main-error={mode} quiet={int(quiet)} exit=1 cleanup_checked", flush=True)
    summary = dict(runtime_runs=rows, invalid_cli_cases=len(cases), invalid_and_hook_processes=2 * len(cases),
                   help_processes=2, forbidden_hook_control_exit=99, parser_boundary_cases=len(boundaries),
                   formatter_cases=8, main_error_cases=6, outcome="PASS", exit_code=0)
    print(f"CLI/log PASS: runtime={len(rows)} invalid={len(cases)} (production+hook), help=2, "
          f"parser_bounds={len(boundaries)}, formatter=8, main_errors=6", flush=True)
    return summary


if __name__ == "__main__":
    run_suite()
