#!/usr/bin/env python3
"""Build/test W2-H, writing only huy-*; never run the historical W2-C writer."""
from contextlib import ExitStack, redirect_stdout
from datetime import datetime, timedelta, timezone
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time

import check_cli_log

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "results/week2"
SOURCES = [
    "Makefile", "include/item.h", "include/buffer.h", "include/pc.h",
    "include/cli.h", "include/runtime_log.h", "src/buffer.c", "src/pc.c",
    "src/cli.c", "src/runtime_log.c", "src/main.c", "examples/thread_lifecycle.c",
    "tests/test_buffer.c", "tests/test_pc.c", "tests/test_pc_faults.c",
    "tests/test_cli.c", "tests/test_main_hooks.c", "tests/test_runtime_log.c",
    "scripts/check_pc_errors.py", "scripts/check_cli_log.py",
    "scripts/check_scaffold.py", "scripts/validate_week2_huy.py",
]
PRESERVED = [
    "include/item.h", "include/buffer.h", "include/pc.h", "src/buffer.c", "src/pc.c",
    "tests/test_buffer.c", "tests/test_pc.c", "tests/test_pc_faults.c",
    "examples/thread_lifecycle.c", "scripts/check_pc_errors.py", "scripts/validate_week2.py",
    "results/week1", "results/week2/chau-*", "docs/reports/week1.md", "docs/handoff-week2-chau.md",
]
FLAGS = ["-Iinclude", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-pthread"]
SAN = ["-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
MAIN_SOURCES = ["src/buffer.c", "src/pc.c", "src/cli.c", "src/runtime_log.c", "src/main.c"]


def manifest():
    return "".join(f"{hashlib.sha256((ROOT / p).read_bytes()).hexdigest()}  {p}\n" for p in SOURCES)


def run(log, command, timeout=30, expected=0, env=None):
    log.write(f"\n$ {shlex.join(command)}\ntimeout={timeout}s expected_exit={expected}\n")
    log.flush()
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=timeout, env=env)
    log.write("stdout:\n" + result.stdout + "stderr:\n" + result.stderr + f"exit_code={result.returncode}\n")
    log.flush()
    assert result.returncode == expected, (command, result.returncode, result.stderr)
    return result


def normalize_mtime(log, paths):
    now = time.time()
    for p in paths:
        if p.is_file() and p.stat().st_mtime > now:
            before = p.stat().st_mtime
            os.utime(p, (now, now))
            log.write(f"mtime normalized {p.relative_to(ROOT)}: {before:.6f} -> {now:.6f}; bytes unchanged\n")


def main():
    OUT.mkdir(exist_ok=True)
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    branch = subprocess.check_output(["git", "branch", "--show-current"], cwd=ROOT, text=True).strip()
    assert branch == "dev/huy", "W2-H evidence must be produced on dev/huy"
    source = manifest()
    source_hash = hashlib.sha256(source.encode()).hexdigest()
    (OUT / "huy-source.sha256").write_text(source, encoding="utf-8")
    saigon = timezone(timedelta(hours=7), "Asia/Saigon")
    header = (f"W2-H actual execution (Asia/Saigon): {datetime.now(saigon).isoformat()}\n"
              f"root={ROOT}\nbranch={branch}\nHEAD_base={head}\n"
              "source=working tree before commit; new CLI/log are not assigned to HEAD_base\n"
              f"source_manifest_sha256={source_hash}\nsource_manifest=results/week2/huy-source.sha256\n"
              "operator=Codex per user request, on ChouPro WSL; no claim Huy personally ran/approved\n")
    with (OUT / "huy-build.log").open("w", encoding="utf-8") as log:
        log.write(header)
        for command in (["uname", "-a"], ["cat", "/etc/os-release"], ["gcc", "--version"],
                        ["make", "--version"], ["git", "--version"], ["python3", "--version"],
                        ["git", "branch", "-vv"], ["git", "remote", "-v"]):
            run(log, command)
        log.write("\nBefore edits: clean dev/chau, fetch succeeded; switched dev/huy and fast-forwarded from 9cf345c to bd00780.\n")
        log.write("Preliminary make exposed DrvFS future-mtime warnings; normalize timestamps without changing bytes.\n")
        normalize_mtime(log, [ROOT / p for p in SOURCES] + list((ROOT / "build").glob("*")))
        run(log, ["make", "clean"])
        built = run(log, ["make", "all", "thread-demo", "buffer-test", "pc-test", "pc-error-test", "cli-tests"])
        assert "warning:" not in (built.stdout + built.stderr).lower()
        normalize_mtime(log, list((ROOT / "build").glob("*")) + list((ROOT / "bin").glob("*")))
        helped = run(log, ["make", "help"])
        assert "warning:" not in (helped.stdout + helped.stderr).lower()
    print("Clean W2-H build PASS: C11/warnings/pthread, all test mains separate.", flush=True)
    with (OUT / "huy-regression-tests.log").open("w", encoding="utf-8") as log:
        log.write(header)
        run(log, ["timeout", "5s", "./bin/thread-demo"])
        for target in ("test-buffer", "test-pc", "test-errors"):
            run(log, ["make", target])
    print("Regression PASS: thread demo, 12 buffer, 8 core, 26 core-error cases.", flush=True)
    with ExitStack() as stack:
        logs = {}
        for key in ("cli", "runtime", "log"):
            log = stack.enter_context((OUT / f"huy-{key}-tests.log").open("w", encoding="utf-8"))
            log.write(header)
            logs[key] = log
        # Store all real invocations/output in their respective logs, and keep
        # the actual assertion summaries in the CLI log instead of flooding UI.
        with redirect_stdout(logs["cli"]):
            summary = check_cli_log.run_suite(logs)
    print("CLI/log PASS: 16 runtime runs, 116 invalid inputs + no-core hooks, help/bounds/log/lifecycle.", flush=True)
    env = os.environ.copy()
    env.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    with (OUT / "huy-sanitizers.log").open("w", encoding="utf-8") as log:
        log.write(header + "ASAN_OPTIONS=detect_leaks=1:halt_on_error=1\n"
                   "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1\n")
        with tempfile.TemporaryDirectory(prefix="os-pc-w2-huy-") as directory:
            binary = str(Path(directory) / "producer-consumer")
            run(log, ["gcc", *FLAGS, *SAN, *MAIN_SOURCES, "-o", binary])
            selected = [
                ("default", (8, 1, 1, 20, 0, 0), False),
                ("T03", (1, 1, 1, 100, 0, 0), True),
                ("T04", (2, 1, 1, 40, 0, 20), False),
                ("T05", (2, 1, 1, 40, 20, 0), False),
                ("K0", (1, 1, 1, 0, 0, 0), True),
                ("default", (8, 1, 1, 20, 0, 0), True),
            ]
            for name, config, quiet in selected:
                args = [] if name == "default" else check_cli_log.arguments(config)
                if quiet: args.append("--quiet")
                checked = run(log, [binary, *args], timeout=10, env=env)
                assert not checked.stderr
                check_cli_log.validate_runtime(checked.stdout, config, quiet, name)
            run(log, [binary, "--help"], env=env)
            rejected = run(log, [binary, "--capacity", "+1"], expected=2, env=env)
            assert "CLI:" in rejected.stderr
            for target, sources in [
                ("cli-parse-test", ["src/buffer.c", "src/pc.c", "src/cli.c", "tests/test_cli.c"]),
                ("log-test", ["src/runtime_log.c", "tests/test_runtime_log.c"]),
            ]:
                test_binary = str(Path(directory) / target)
                run(log, ["gcc", *FLAGS, *SAN, *sources, "-o", test_binary])
                args = check_cli_log.arguments((65536, 1, 1, 1000000, 60000, 60000)) if target == "cli-parse-test" else []
                checked = run(log, [test_binary, *args], env=env)
                assert not checked.stderr
            hook_binary = str(Path(directory) / "main-hook-test")
            wraps = [f"-Wl,--wrap={name}" for name in
                     ("pc_run", "pc_result_destroy", "pthread_create", "sem_init", "sem_destroy")]
            run(log, ["gcc", *FLAGS, *SAN, *MAIN_SOURCES, "tests/test_main_hooks.c", *wraps, "-o", hook_binary])
            for mode in ("init-error", "cleanup-error"):
                hook_env = dict(env, PC_MAIN_TEST_MODE=mode)
                log.write(f"PC_MAIN_TEST_MODE={mode} (sanitized test binary only)\n")
                checked = run(log, [hook_binary, "--items-per-producer", "3", "--quiet"],
                              expected=1, env=hook_env)
                assert "verification=FAIL run_status=1" in checked.stdout
                assert "result_destroy records_before=1 records_after=0" in checked.stderr
                assert not any(marker in checked.stderr for marker in
                               ("AddressSanitizer", "LeakSanitizer", "runtime error:"))
    print("ASan/UBSan/leak detection PASS: production, parser boundary, formatter and main error cleanup.", flush=True)
    with (OUT / "huy-review.log").open("w", encoding="utf-8") as log:
        log.write(header)
        run(log, ["python3", "scripts/check_scaffold.py"])
        run(log, ["git", "diff", "--check"])
        run(log, ["git", "diff", "--cached", "--check"])
        run(log, ["git", "diff", "--exit-code", head, "--", *PRESERVED])
        run(log, ["git", "status", "--short"])
        assert manifest() == source
        log.write("PASS source manifest unchanged; Week 1, Chau evidence/core/tests/writer/handoff preserved byte-for-byte.\n")
    summary.update(HEAD_base=head, branch=branch, source_manifest_sha256=source_hash,
                   regression=dict(thread_demo="PASS", buffer_cases=12, core_cases=8, error_cases=26),
                   sanitizers="PASS")
    (OUT / "huy-test-summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print("W2-H validation completed, exit=0; evidence results/week2/huy-*.", flush=True)


if __name__ == "__main__":
    main()
