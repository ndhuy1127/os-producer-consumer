#!/usr/bin/env python3
"""Validate project scaffolding only; this does not test the C algorithm."""
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = [
    "README.md", ".gitignore", "Makefile", ".github/PULL_REQUEST_TEMPLATE.md",
    "src/README.md", "include/README.md", "tests/README.md", "scripts/README.md",
    "docs/plan.md", "docs/design.md", "docs/test-plan.md", "docs/progress.md",
    "docs/reports/README.md", "docs/final-report/README.md", "docs/slides/README.md",
    *[f"docs/reports/week{n}.md" for n in range(1, 5)],
    *[f"results/week{n}/README.md" for n in range(1, 5)],
]
errors = []
for rel in REQUIRED:
    if not (ROOT / rel).is_file():
        errors.append(f"Missing file: {rel}")
markdown = [p for p in ROOT.rglob("*.md") if ".git" not in p.relative_to(ROOT).parts]
links = 0
for document in markdown:
    for target in re.findall(r"!?\[[^\]]*\]\(([^)]+)\)", document.read_text(encoding="utf-8")):
        target = target.strip().split()[0].strip("<>")
        if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target) or target.startswith("#"):
            continue
        target = unquote(target.split("#", 1)[0])
        links += 1
        if not (document.parent / target).exists():
            errors.append(f"Broken link in {document.relative_to(ROOT)}: {target}")
for document in ROOT.rglob("*"):
    if document.is_dir() and document.name.casefold() == "meetings":
        errors.append(f"Unexpected directory: {document.relative_to(ROOT)}")
probes = ["AGENTS.md", "src/AGENTS.md", "docs/reports/AGENTS.md",
          "build/probe.o", "bin/producer-consumer", ".env", "private/probe.txt",
          "src/probe.o", ".cache/probe", "src/probe.tmp", "secret.key"]
kept = ["results/week1/evidence.log", "results/week2/table.csv", "results/week3/image.png",
        "docs/reports/week4.md"]
try:
    for probe, expected in [(p, 0) for p in probes] + [(p, 1) for p in kept]:
        result = subprocess.run(["git", "check-ignore", "-q", "--", probe], cwd=ROOT)
        if result.returncode != expected:
            errors.append(f"Ignore mismatch: {probe}, exit {result.returncode}")
    for args in (["ls-files", "-z"], ["diff", "--cached", "--name-only", "-z"]):
        result = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, check=True)
        if any(Path(p).name == "AGENTS.md" for p in result.stdout.decode("utf-8").split("\0") if p):
            errors.append("AGENTS.md appears in tracked/staged files")
except (OSError, subprocess.CalledProcessError) as exc:
    errors.append(f"Git validation unavailable: {type(exc).__name__}")
if errors:
    print("\n".join(errors), file=sys.stderr)
    raise SystemExit(1)
print(f"Scaffold verified: {len(REQUIRED)} required files; {links} relative links; ignore rules; no tracked/staged AGENTS.md.")
print("No C algorithm, thread behavior or academic task has been validated.")
