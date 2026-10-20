#!/usr/bin/env python3
"""Build/test Week 3 (several producers/consumers); write evidence to results/week3 only.

Clean build without warnings, regression (thread demo, buffer, core presets,
fault injection), CLI/log suite, repeated stress runs, ASan/UBSan with leak
detection and ThreadSanitizer, then scaffold/diff/hash review. Week 1/2
evidence is never rewritten.
"""
from contextlib import ExitStack, redirect_stdout
from datetime import datetime, timedelta, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile
import time

import check_cli_log
import check_stress

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "results/week3"
SOURCES = [
    "Makefile", "include/item.h", "include/buffer.h", "include/pc.h",
    "include/cli.h", "include/runtime_log.h", "src/buffer.c", "src/pc.c",
    "src/cli.c", "src/runtime_log.c", "src/main.c", "examples/thread_lifecycle.c",
    "tests/test_buffer.c", "tests/test_pc.c", "tests/test_pc_faults.c",
    "tests/test_cli.c", "tests/test_main_hooks.c", "tests/test_runtime_log.c",
    "scripts/check_pc_errors.py", "scripts/check_cli_log.py", "scripts/check_stress.py",
    "scripts/check_scaffold.py", "scripts/validate_week3.py",
]
FLAGS = ["-Iinclude", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-pthread"]
ASAN = ["-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
TSAN = ["-g", "-O1", "-fsanitize=thread"]
MAIN_SOURCES = ["src/buffer.c", "src/pc.c", "src/cli.c", "src/runtime_log.c", "src/main.c"]
CORE_SOURCES = ["src/buffer.c", "src/pc.c", "tests/test_pc.c"]
SANITIZED_RUNS = [  # name, (N,P,C,K,Dp_ms,Dc_ms), quiet
    ("T06a", (3, 3, 2, 100, 0, 0), False),
    ("T06b", (2, 2, 5, 100, 0, 0), False),
    ("T09a", (1, 1, 4, 0, 0, 0), False),
    ("T09b", (2, 2, 3, 30, 0, 10), False),
    ("T04-multi", (2, 3, 2, 40, 0, 20), True),
    ("T05-multi", (2, 2, 3, 40, 20, 0), True),
    ("N1-8P8C", (1, 8, 8, 500, 0, 0), True),
    ("max-workers", (64, 64, 64, 100, 0, 0), True),
    ("T07", (16, 4, 3, 10000, 0, 0), True),
]
SANITIZER_MARKERS = ("AddressSanitizer", "LeakSanitizer", "ThreadSanitizer", "runtime error:")


def manifest():
    return "".join(f"{hashlib.sha256((ROOT / p).read_bytes()).hexdigest()}  {p}\n" for p in SOURCES)


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def run(log, command, timeout=60, expected=0, env=None):
    log.write(f"\n$ {shlex.join(command)}\ntimeout={timeout}s expected_exit={expected}\n")
    log.flush()
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=timeout, env=env)
    log.write("stdout:\n" + result.stdout + "stderr:\n" + result.stderr + f"exit_code={result.returncode}\n")
    log.flush()
    assert result.returncode == expected, (command, result.returncode, result.stderr)
    return result


def normalize_mtime(log, paths):
    """DrvFS (/mnt/*) can report future mtimes; fix timestamps only, never bytes."""
    now = time.time()
    for p in paths:
        if p.is_file() and p.stat().st_mtime > now:
            before = p.stat().st_mtime
            os.utime(p, (now, now))
            log.write(f"mtime normalized {p.relative_to(ROOT)}: {before:.6f} -> {now:.6f}; bytes unchanged\n")


def sanitized(log, binary, config, quiet, name, env, prefix=()):
    args = check_cli_log.arguments(config) + (["--quiet"] if quiet else [])
    checked = run(log, [*prefix, binary, *args], timeout=180, env=env)
    assert not checked.stderr and not any(m in checked.stdout for m in SANITIZER_MARKERS)
    check_cli_log.validate_runtime(checked.stdout, config, quiet, name)
    log.write(f"PASS sanitized {name} quiet={int(quiet)}: oracle/summary verified, no diagnostics\n")


def results_table(stamp, source_hash, head, runtime_rows, stress_rows, sanitizer_rows):
    lines = [
        "# Bảng kết quả Tuần 3",
        "",
        f"Sinh tự động bởi `scripts/validate_week3.py` lúc {stamp} (Asia/Saigon), mã nguồn tại commit "
        f"`{head}`, manifest SHA256 `{source_hash}`. Mọi số liệu là của lần chạy này; số lần chờ, "
        "số snapshot và cách chia DATA giữa các consumer phụ thuộc bộ lập lịch, không phải hằng số.",
        "",
        "## Một lần chạy qua chương trình chính (thường và quiet)",
        "",
        "Cả hai chế độ đều exit 0 và in `verification=PASS`. Ở chế độ thường, script dựng lại hàng đợi "
        "tuyến tính từ từng dòng log để kiểm tra mọi ID đúng một lần, FIFO, chủ sở hữu ID và thứ tự trong "
        "từng producer, count trong [0,N], mỗi consumer nhận đúng một STOP. Cột số liệu lấy từ lần chạy thường.",
        "",
        "| Ca | N/P/C/K/Dp/Dc | Thường / quiet | Bản ghi | count tối đa | Snapshot đầy / rỗng "
        "| WAIT_SLOT / WAIT_ITEM | DATA còn đợi khi gửi STOP | DATA từng consumer |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    by_name = {}
    for row in runtime_rows:
        by_name.setdefault(row["name"], {})[row["quiet"]] = row
    for name, pair in by_name.items():
        normal, quiet = pair[False], pair[True]
        lines.append(
            f"| {name} | {'/'.join(map(str, normal['config']))} | {normal['outcome']} / {quiet['outcome']} "
            f"| {normal['records_retained']} | {normal['max_count']} "
            f"| {normal['full_snapshots']} / {normal['empty_snapshots']} "
            f"| {normal['wait_slot']} / {normal['wait_item']} | {normal['pending_after_producer_join']} "
            f"| {', '.join(map(str, normal['consumer_data']))} |")
    lines += [
        "",
        "## Chạy lặp, mỗi lần kiểm tra bằng oracle đầy đủ",
        "",
        "Khoảng giá trị là nhỏ nhất–lớn nhất qua các lần chạy.",
        "",
        check_stress.table(stress_rows).rstrip("\n"),
        "",
        "## Sanitizer",
        "",
        "| Công cụ | Chương trình | Cấu hình | Kết quả |",
        "| --- | --- | --- | --- |",
    ]
    lines += [f"| {tool} | {target} | {what} | {outcome} |" for tool, target, what, outcome in sanitizer_rows]
    return "\n".join(lines) + "\n"


def main():
    OUT.mkdir(exist_ok=True)
    head = git("rev-parse", "HEAD")
    branch = git("branch", "--show-current")
    dirty = git("status", "--porcelain", "--", *SOURCES)
    source = manifest()
    source_hash = hashlib.sha256(source.encode()).hexdigest()
    (OUT / "source.sha256").write_text(source, encoding="utf-8")
    saigon = timezone(timedelta(hours=7), "Asia/Saigon")
    stamp = datetime.now(saigon).isoformat(timespec="seconds")
    header = (f"Week 3 actual execution (Asia/Saigon): {stamp}\n"
              f"root={ROOT}\nbranch={branch}\nHEAD={head}\n"
              f"source={'working tree with uncommitted changes on HEAD' if dirty else 'exactly HEAD (sources clean)'}\n"
              f"source_manifest_sha256={source_hash}\nsource_manifest=results/week3/source.sha256\n"
              "operator=Claude Code theo yeu cau Huy, may cua Huy (WSL Ubuntu-24.04); "
              "W3-C chua duoc Chau review/xac nhan\n")
    with (OUT / "build.log").open("w", encoding="utf-8") as log:
        log.write(header)
        for command in (["uname", "-a"], ["cat", "/etc/os-release"], ["gcc", "--version"],
                        ["make", "--version"], ["git", "--version"], ["python3", "--version"],
                        ["nproc"], ["git", "branch", "-vv"]):
            run(log, command)
        normalize_mtime(log, [ROOT / p for p in SOURCES])
        run(log, ["make", "clean"])
        built = run(log, ["make", "all", "thread-demo", "buffer-test", "pc-test", "pc-error-test", "cli-tests"],
                    timeout=300)
        assert "warning:" not in (built.stdout + built.stderr).lower()
        normalize_mtime(log, list((ROOT / "build").glob("*")) + list((ROOT / "bin").glob("*")))
        helped = run(log, ["make", "help"])
        assert "warning:" not in (helped.stdout + helped.stderr).lower()
    print("Clean build PASS: C11, -Wall -Wextra -Wpedantic, -pthread, no warnings.", flush=True)

    with (OUT / "core-tests.log").open("w", encoding="utf-8") as log:
        log.write(header)
        run(log, ["timeout", "5s", "./bin/thread-demo"])
        buffer = run(log, ["make", "test-buffer"])
        core = run(log, ["make", "test-pc"], timeout=120)
        errors = run(log, ["make", "test-errors"], timeout=300)
        assert "Sequential buffer tests: 12/12 PASS" in buffer.stdout
        assert "Core preset tests: 16/16 PASS" in core.stdout
        assert "Error tests: 32/32 PASS" in errors.stdout
    print("Core PASS: thread demo, buffer 12/12, presets 16/16, fault injection 32/32.", flush=True)

    with ExitStack() as stack:
        logs = {}
        for key in ("cli", "runtime", "log"):
            log = stack.enter_context((OUT / f"{key}-tests.log").open("w", encoding="utf-8"))
            log.write(header)
            logs[key] = log
        with redirect_stdout(logs["cli"]):
            cli_summary = check_cli_log.run_suite(logs)
    print(f"CLI/log PASS: {len(cli_summary['runtime_runs'])} production runs, "
          f"{cli_summary['invalid_cli_cases']} invalid inputs + hooks.", flush=True)

    with (OUT / "stress-tests.log").open("w", encoding="utf-8") as log:
        log.write(header)
        with redirect_stdout(log):
            stress_rows = check_stress.run_stress(20, log)
            print(check_stress.table(stress_rows), end="")
    print(f"Stress PASS: {sum(r['runs'] for r in stress_rows)} repeated runs.", flush=True)

    sanitizer_rows = []
    asan_env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                    UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    tsan_env = dict(os.environ, TSAN_OPTIONS="halt_on_error=1 second_deadlock_stack=1")
    # TSan needs low-entropy ASLR on this WSL2 kernel ("unexpected memory
    # mapping" otherwise); setarch -R disables ASLR for that one process only.
    no_aslr = ("setarch", platform.machine(), "-R")
    with (OUT / "sanitizers.log").open("w", encoding="utf-8") as log, \
            tempfile.TemporaryDirectory(prefix="os-pc-w3-") as directory:
        log.write(header + f"ASAN_OPTIONS={asan_env['ASAN_OPTIONS']}\n"
                  f"UBSAN_OPTIONS={asan_env['UBSAN_OPTIONS']}\nTSAN_OPTIONS={tsan_env['TSAN_OPTIONS']}\n"
                  f"TSan prefix: {' '.join(no_aslr)} (ASLR off for the sanitized process only)\n")
        for tool, flags, env, prefix in (("ASan+UBSan", ASAN, asan_env, ()),
                                         ("TSan", TSAN, tsan_env, no_aslr)):
            program = str(Path(directory) / f"producer-consumer-{tool[0].lower()}san")
            runner = str(Path(directory) / f"pc-test-{tool[0].lower()}san")
            built = run(log, ["gcc", *FLAGS, *flags, *MAIN_SOURCES, "-o", program], timeout=300)
            assert "warning:" not in built.stderr.lower()
            built = run(log, ["gcc", *FLAGS, *flags, *CORE_SOURCES, "-o", runner], timeout=300)
            assert "warning:" not in built.stderr.lower()
            for name, config, quiet in SANITIZED_RUNS:
                sanitized(log, program, config, quiet, name, env, prefix)
            checked = run(log, [*prefix, runner, "all"], timeout=600, env=env)
            assert "Core preset tests: 16/16 PASS" in checked.stdout
            assert not any(m in checked.stdout + checked.stderr for m in SANITIZER_MARKERS)
            names = ", ".join(name for name, _, _ in SANITIZED_RUNS)
            sanitizer_rows.append((tool, "bin/producer-consumer", names, "PASS, không có cảnh báo"))
            sanitizer_rows.append((tool, "pc-test all", "16 preset lõi", "PASS, không có cảnh báo"))
            print(f"{tool} PASS: {len(SANITIZED_RUNS)} production configs + 16 core presets.", flush=True)

    with (OUT / "review.log").open("w", encoding="utf-8") as log:
        log.write(header)
        run(log, ["python3", "scripts/check_scaffold.py"])
        run(log, ["git", "diff", "--check"])
        run(log, ["git", "diff", "--exit-code", "HEAD", "--", "results/week1", "results/week2"])
        run(log, ["git", "status", "--short"])
        assert manifest() == source
        log.write("PASS source manifest unchanged during the run; results/week1 and results/week2 untouched.\n")

    (OUT / "results-table.md").write_text(
        results_table(stamp, source_hash, head, cli_summary["runtime_runs"], stress_rows, sanitizer_rows),
        encoding="utf-8")
    summary = dict(executed_at=stamp, HEAD=head, branch=branch, sources_clean=not dirty,
                   source_manifest_sha256=source_hash,
                   core=dict(thread_demo="PASS", buffer_cases=12, core_presets=16, fault_cases=32),
                   cli=cli_summary, stress=stress_rows,
                   sanitizers=[dict(tool=t, target=g, configs=w, outcome=o) for t, g, w, o in sanitizer_rows])
    (OUT / "test-summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n",
                                           encoding="utf-8")
    print("Week 3 validation completed, exit=0; evidence in results/week3.", flush=True)


if __name__ == "__main__":
    main()
