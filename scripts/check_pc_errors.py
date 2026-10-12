#!/usr/bin/env python3
"""Run injected errors in isolated processes with deadlines; no CLI claims."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
RECOVERABLE = [
    "wait-eintr", "trywait-eintr", "producer-create", "consumer-create",
    "init-empty", "init-full", "init-guard", "malloc", "calloc-record",
    "calloc-slot", "calloc-produced", "calloc-consumed", "destroy",
    "config-N0", "config-Nbig", "config-P2", "config-C2", "config-Kbig", "config-delay",
]
FATAL = {
    "fatal-guard": "sem_wait guard",
    "fatal-wait": "sem_wait permission",
    "fatal-trywait": "sem_trywait permission",
    "fatal-post": "sem_post full after enqueue",
    "fatal-join": "pthread_join producer",
    "fatal-push": "buffer_push: BUFFER_INVALID_ARGUMENT",
    "fatal-pop": "buffer_pop: BUFFER_INVALID_ARGUMENT",
}


def main():
    failed = 0
    for fault in [*RECOVERABLE, *FATAL]:
        expected = 1 if fault in FATAL else 0
        command = ["./bin/pc-error-test", fault]
        print(f"$ {' '.join(command)} (timeout=5s expected_exit={expected})", flush=True)
        try:
            run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=5)
        except subprocess.TimeoutExpired:
            print(f"FAIL fault={fault}: timeout", flush=True)
            failed += 1
            continue
        print(run.stdout, end="", flush=True)
        print(run.stderr, end="", flush=True)
        ok = run.returncode == expected
        if fault in FATAL:
            ok = ok and "fatal:" in run.stderr and FATAL[fault] in run.stderr
        else:
            ok = ok and f"PASS fault={fault}" in run.stdout
        print(f"{'PASS' if ok else 'FAIL'} process_exit={run.returncode} expected_exit={expected}", flush=True)
        failed += not ok
    total = len(RECOVERABLE) + len(FATAL)
    print(f"Error tests: {total - failed}/{total} {'PASS' if not failed else 'FAIL'}")
    return int(bool(failed))


if __name__ == "__main__":
    sys.exit(main())
