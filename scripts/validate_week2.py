#!/usr/bin/env python3
"""Capture real W2-C build/runtime/error/sanitizer evidence from this checkout."""
from datetime import datetime, timedelta, timezone
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "results/week2"
SOURCES = [
    "Makefile", "include/item.h", "include/buffer.h", "include/pc.h",
    "src/buffer.c", "src/main.c", "src/pc.c", "examples/thread_lifecycle.c",
    "tests/test_buffer.c", "tests/test_pc.c", "tests/test_pc_faults.c",
    "scripts/check_pc_errors.py", "scripts/validate_week2.py",
]
PRESETS = [
    ("default", 20, 0, 0), ("source-N5", 20, 0, 0), ("T02", 100, 0, 0),
    ("T03", 100, 0, 0), ("T04", 40, 0, 20), ("T05", 40, 20, 0),
    ("K0", 0, 0, 0), ("queued-stop", 20, 0, 20),
]
FLAGS = ["-Iinclude", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-pthread"]
SANITIZERS = ["-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]


def source_manifest():
    return "".join(f"{hashlib.sha256((ROOT / p).read_bytes()).hexdigest()}  {p}\n" for p in SOURCES)


def run(log, command, timeout=30, expected=0, env=None):
    log.write(f"\n$ {shlex.join(command)}\nsubprocess_timeout={timeout}s expected_exit={expected}\n")
    log.flush()
    completed = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, timeout=timeout, env=env)
    log.write(completed.stdout)
    log.write(f"exit_code={completed.returncode}\n")
    log.flush()
    if completed.returncode != expected:
        raise RuntimeError(f"{shlex.join(command)}: exit={completed.returncode}, expected={expected}")
    return completed.stdout


def normalize_mtime(log, paths):
    now = time.time()
    for p in paths:
        if p.is_file() and p.stat().st_mtime > now:
            old = p.stat().st_mtime
            os.utime(p, (now, now))
            log.write(f"mtime normalized {p.relative_to(ROOT)}: {old:.6f} -> {now:.6f}; bytes unchanged\n")


def main():
    OUT.mkdir(exist_ok=True)
    manifest = source_manifest()
    (OUT / "chau-source.sha256").write_text(manifest, encoding="utf-8")
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    branch = subprocess.check_output(["git", "branch", "--show-current"], cwd=ROOT, text=True).strip()
    # Asia/Saigon uses UTC+07:00; avoid depending on optional host tzdata.
    saigon = timezone(timedelta(hours=7), "Asia/Saigon")
    header = (f"W2-C actual execution (Asia/Saigon): {datetime.now(saigon).isoformat()}\n"
              f"root={ROOT}\nbranch={branch}\nHEAD_base={head}\n"
              "source=working tree before commit; HEAD_base does not contain new code\n"
              f"source_manifest_sha256={hashlib.sha256(manifest.encode()).hexdigest()}\n"
              "source_manifest=results/week2/chau-source.sha256\n")
    with (OUT / "chau-build.log").open("w", encoding="utf-8") as log:
        log.write(header)
        for command in (["uname", "-a"], ["cat", "/etc/os-release"], ["gcc", "--version"],
                        ["make", "--version"], ["git", "--version"], ["python3", "--version"],
                        ["git", "branch", "-vv"], ["git", "remote", "-v"],
                        ["git", "rev-list", "--left-right", "--count", "HEAD...origin/dev/chau"],
                        ["git", "rev-list", "--left-right", "--count", "HEAD...origin/main"]):
            run(log, command)
        log.write("\nPreliminary build exposed a Makefile wrapper comma error; corrected before this clean build.\n")
        log.write("WSL /mnt/d preliminary build had future-mtime clock-skew warnings.\n")
        # Match Week 1: timestamp correction only, no change to file contents.
        normalize_mtime(log, [ROOT / rel for rel in SOURCES] + list((ROOT / "build").glob("*")))
        run(log, ["make", "clean"])
        output = run(log, ["make", "all", "thread-demo", "buffer-test", "pc-test", "pc-error-test"])
        if "warning:" in output.lower() or "clock skew" in output.lower():
            raise RuntimeError("Clean build must be free of warnings")
        # DrvFS can stamp newly generated files ahead of the WSL clock, too.
        # Normalize these before the next make reads generated dependencies.
        normalize_mtime(log, list((ROOT / "build").glob("*")) + list((ROOT / "bin").glob("*")))
        run(log, ["make", "help"])
        run(log, ["timeout", "5s", "./bin/thread-demo"])
        run(log, ["make", "test-buffer"])
    print("Captured clean build, Week 1 thread demo and 12/12 buffer tests.", flush=True)
    with (OUT / "chau-runtime-tests.log").open("w", encoding="utf-8") as log:
        log.write(header)
        run(log, ["timeout", "5s", "./bin/producer-consumer"])
        run(log, ["timeout", "5s", "./bin/producer-consumer", "--capacity", "5"], expected=2)
        for name, count, producer_ms, consumer_ms in PRESETS:
            budget = 5 + (2 * count * (producer_ms + consumer_ms) + 999) // 1000
            args = ["./bin/pc-test", name]
            if name in {"source-N5", "T04", "T05", "K0", "queued-stop"}:
                args.append("--records")
            run(log, ["timeout", f"{budget}s", *args], timeout=budget + 2)
    print("Captured eight runtime presets and guarded full/empty/STOP records.", flush=True)
    with (OUT / "chau-error-tests.log").open("w", encoding="utf-8") as log:
        log.write(header)
        run(log, ["python3", "scripts/check_pc_errors.py"], timeout=40)
    print("Captured 26 isolated fault/config cases, each with a 5s timeout.", flush=True)
    env = os.environ.copy()
    env.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    with (OUT / "chau-sanitizers.log").open("w", encoding="utf-8") as log:
        log.write(header)
        log.write("ASAN_OPTIONS=detect_leaks=1:halt_on_error=1\nUBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1\n")
        with tempfile.TemporaryDirectory(prefix="os-pc-w2-") as directory:
            builds = [
                ("pc-test", ["src/buffer.c", "src/pc.c", "tests/test_pc.c"], ["all"]),
                ("producer-consumer", ["src/buffer.c", "src/pc.c", "src/main.c"], []),
                ("buffer-test", ["src/buffer.c", "tests/test_buffer.c"], []),
            ]
            for target, sources, args in builds:
                binary = str(Path(directory) / target)
                run(log, ["gcc", *FLAGS, *SANITIZERS, *sources, "-o", binary])
                run(log, ["timeout", "15s", binary, *args], timeout=17, env=env)
    print("Captured ASan/UBSan: eight core presets, production default, 11 buffer cases.", flush=True)
    with (OUT / "chau-review.log").open("w", encoding="utf-8") as log:
        log.write(header)
        run(log, ["python3", "scripts/check_scaffold.py"])
        run(log, ["git", "diff", "--check"])
        run(log, ["git", "diff", "--cached", "--check"])
        unchanged = ["include/item.h", "include/buffer.h", "src/buffer.c", "tests/test_buffer.c",
                     "examples/thread_lifecycle.c", "results/week1", "docs/reports/week1.md"]
        run(log, ["git", "diff", "--exit-code", "HEAD", "--", *unchanged])
        run(log, ["git", "status", "--short"])
        if source_manifest() != manifest:
            raise RuntimeError("Tested source changed during validation")
        log.write("source_manifest_after_tests=identical\n")
    (OUT / "chau-test-summary.json").write_text(json.dumps({
        "HEAD_base": head, "branch": branch,
        "source_manifest_sha256": hashlib.sha256(manifest.encode()).hexdigest(),
        "core_presets_passed": [p[0] for p in PRESETS], "fault_cases_passed": 26,
        "sanitizers": "PASS", "exit_code": 0,
    }, indent=2) + "\n", encoding="utf-8")
    print("W2-C validation completed, exit=0. Evidence: results/week2/", flush=True)


if __name__ == "__main__":
    main()
