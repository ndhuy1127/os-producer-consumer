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
]
FLAGS = ["--capacity", "--producers", "--consumers", "--items-per-producer",
         "--producer-delay-ms", "--consumer-delay-ms"]
RECORD = re.compile(
    r"op_seq=(\d+) op=(ENQUEUE|DEQUEUE) actor=(P0|C0|MAIN) kind=(DATA|STOP) "
    r"data_id=(\d+|-) value=(-?\d+) enqueue_seq=(\d+) count_after=(\d+) "
    r"capacity=(\d+) wait_reason=(NONE|WAIT_SLOT|WAIT_ITEM)"
)


def arguments(config):
    return [token for flag, value in zip(FLAGS, config) for token in (flag, str(value))]


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
    assert len(found) == 1, (pattern, lines)
    return found[0].groups()


def validate_runtime(stdout, config, quiet, name):
    n, p, c, k, dp, dc = config
    lines = stdout.splitlines()
    actual_config = matched(lines, r"config: N=(\d+) P=(\d+) C=(\d+) K=(\d+) Dp_ms=(\d+) Dc_ms=(\d+)")
    assert tuple(map(int, actual_config)) == config
    data = tuple(map(int, matched(lines, r"DATA: enqueued=(\d+) dequeued=(\d+) processed=(\d+) missing=(\d+) duplicate=(\d+) out_of_range=(\d+) value_errors=(\d+) FIFO_errors=(\d+)")))
    buffer = tuple(map(int, matched(lines, r"buffer: min=(\d+) max=(\d+) final=(\d+) count_errors=(\d+); STOP: enqueued=(\d+) dequeued=(\d+)")))
    waits = tuple(map(int, matched(lines, r"waits: WAIT_SLOT=(\d+) WAIT_ITEM=(\d+) records=(\d+) data_pending_after_producer_join=(\d+)")))
    workers = matched(lines, r"workers: created=(\d+)P/(\d+)C joined=(\d+)P/(\d+)C; verification=(PASS|FAIL) run_status=(\d+)")
    assert data == (k, k, k, 0, 0, 0, 0, 0)
    assert buffer[0] == 0 and 0 <= buffer[1] <= n and buffer[2:] == (0, 0, 1, 1)
    assert tuple(map(int, workers[:4])) == (1, 1, 1, 1) and workers[4:] == ("PASS", "0")
    assert waits[2] == 2 * (k + 1) and 0 <= waits[3] <= n
    record_lines = [line for line in lines if line.startswith("op_seq=")]
    assert len(lines) == len(record_lines) + 5  # no unknown/incomplete output lines
    result = dict(name=name, quiet=quiet, config=config, exit_code=0, outcome="PASS",
                  records_retained=waits[2], wait_slot=waits[0], wait_item=waits[1],
                  pending_after_producer_join=waits[3], max_count=buffer[1],
                  full_snapshots=None, empty_snapshots=None)
    if quiet:
        assert not record_lines
        if name == "T04": assert buffer[1] == n and waits[0] > 0
        if name == "T05": assert waits[1] > 0
        if name == "queued-stop": assert waits[3] > 0
        return result
    assert len(record_lines) == 2 * (k + 1)
    queue, taken = [], 0  # independent append-only queue, no circular indexes
    enqueued, dequeued = Counter(), Counter()
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
            assert item[1] is not None and 0 <= item[1] < k and value == item[1] % 1000 + 1
        else:
            assert data_id == "-" and value == 0
        if op == "ENQUEUE":
            assert actor == ("P0" if kind == "DATA" else "MAIN")
            assert wait in ("NONE", "WAIT_SLOT") and enqueue_seq == len(queue) + 1
            slots += wait == "WAIT_SLOT"
            producer_slots += actor == "P0" and wait == "WAIT_SLOT"
            queue.append((item, enqueue_seq))
            if kind == "DATA":
                assert stops_in == 0
                enqueued[item[1]] += 1
            else:
                assert sum(enqueued.values()) == k
                stops_in += 1
        else:
            assert actor == "C0" and wait in ("NONE", "WAIT_ITEM") and taken < len(queue)
            assert queue[taken] == (item, enqueue_seq)
            taken += 1
            items += wait == "WAIT_ITEM"
            if kind == "DATA":
                assert stops_out == 0
                dequeued[item[1]] += 1
            else:
                assert sum(dequeued.values()) == k
                stops_out += 1
        assert count == len(queue) - taken and 0 <= count <= n
        full += count == n
        empty += count == 0
        max_count = max(max_count, count)
    expected = Counter(range(k))
    assert enqueued == dequeued == expected and taken == len(queue) and stops_in == stops_out == 1
    assert (slots, items) == waits[:2] and max_count == buffer[1]
    if name == "T04": assert full > 0 and producer_slots > 0
    if name == "T05": assert empty > 0 and items > 0
    if name == "queued-stop": assert waits[3] > 0
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
    for flag, value in [("--capacity", 65537), ("--items-per-producer", 1000001),
                        ("--producer-delay-ms", 60001), ("--consumer-delay-ms", 60001)]:
        cases.append((flag + "-above", [flag, str(value)], flag))
    for flag in FLAGS[1:3]:
        for value in [2, 64, 65, 18446744073709551615]:
            cases.append((flag + f"-unsupported-{value}", [flag, str(value)], "not supported in Week 2"))
    return cases


def run_suite(logs=None):
    logs = logs or {}
    rows = []
    for name, config in PRESETS:
        for quiet in (False, True):
            args = [] if name == "default" else arguments(config)
            if quiet: args.append("--quiet")
            budget = 5 + (2 * config[3] * (config[4] + config[5]) + 999) // 1000
            run = execute(["./bin/producer-consumer", *args], timeout=budget, log=logs.get("runtime"))
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
        assert "only 1 in Week 2" in run.stdout and "verification=" not in run.stdout
    # Positive control: prove the forbidden-core hook is active in this binary.
    run = execute(["./bin/main-hook-test", "--items-per-producer", "0"], expected=99,
                  log=logs.get("cli"), mode="forbid")
    assert "FORBIDDEN pc_run" in run.stderr
    boundaries = [
        ("defaults", [], (8, 1, 1, 20, 0, 0), 0, 0),
        ("lower", arguments((1, 1, 1, 0, 0, 0)), (1, 1, 1, 0, 0, 0), 0, 0),
        ("upper", arguments((65536, 1, 1, 1000000, 60000, 60000)) + ["--quiet"],
         (65536, 1, 1, 1000000, 60000, 60000), 1, 0),
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
    assert not run.stderr and "Log formatter tests: 7/7 PASS" in run.stdout
    for mode in ("init-error", "cleanup-error", "unverified"):
        for quiet in (False, True):
            args = ["--capacity", "1", "--items-per-producer", "3"]
            if quiet: args.append("--quiet")
            run = execute(["./bin/main-hook-test", *args], expected=1, log=logs.get("log"), mode=mode)
            assert "verification=FAIL run_status=1" in run.stdout and "verification=PASS" not in run.stdout
            assert "result_destroy records_before=1 records_after=0" in run.stderr
            if mode == "cleanup-error": assert "pc_run status=1 verified=1" in run.stderr
            if quiet: assert "op_seq=" not in run.stdout
            print(f"PASS main-error={mode} quiet={int(quiet)} exit=1 cleanup_checked", flush=True)
    summary = dict(runtime_runs=rows, invalid_cli_cases=len(cases), invalid_and_hook_processes=2 * len(cases),
                   help_processes=2, forbidden_hook_control_exit=99, parser_boundary_cases=len(boundaries),
                   formatter_cases=7, main_error_cases=6, outcome="PASS", exit_code=0)
    print(f"CLI/log PASS: runtime=16 invalid={len(cases)} (production+hook), help=2, parser_bounds=5, formatter=7, main_errors=6", flush=True)
    return summary


if __name__ == "__main__":
    run_suite()
