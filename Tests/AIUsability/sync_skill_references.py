#!/usr/bin/env python3
"""Mirror the TransitionFX plugin's user-facing documentation and public headers into the
`using-transitionfx` skill's reference/ directory.

Why a copy and not a link: `claude plugin eval` loads only the plugin directory, with nothing
from the surrounding repository, so the skill has to carry the documents itself. Keeping them as
verbatim copies (same repo-relative path under reference/) means the eval measures the plugin's
*real* documentation, and `test_agent_readiness.py` fails when a copy drifts from its source.

Usage (from anywhere):
    python3 Tests/AIUsability/sync_skill_references.py          # copy
    python3 Tests/AIUsability/sync_skill_references.py --check  # exit 1 if any copy is stale
"""
from __future__ import annotations

import filecmp
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[1]
REFERENCE_DIR = HERE / "skills" / "using-transitionfx" / "reference"

# Repo-relative files the skill exposes to the agent. Order is irrelevant; paths are preserved
# under reference/ so relative links inside the Markdown keep resolving.
SOURCE_FILES: list[str] = [
    "README.md",
    "docs/API_Reference_EN.md",
    "docs/QUICKSTART_EN.md",
    "docs/FAQ_EN.md",
    "docs/TransitionFX_PreviewTool_Manual.md",
    "docs/SHOWCASE_LEVEL.md",
    "docs/WIDGET_LAYER_SAMPLE.md",
    "docs/ROADMAP_EN.md",
    "CHANGELOG.md",
]
SOURCE_GLOBS: list[str] = [
    "Plugins/TransitionFX/Source/TransitionFX/Public/*.h",
]


def source_paths() -> list[Path]:
    paths = [REPO_ROOT / rel for rel in SOURCE_FILES]
    for pattern in SOURCE_GLOBS:
        paths.extend(sorted(REPO_ROOT.glob(pattern)))
    missing = [p for p in paths if not p.is_file()]
    if missing:
        raise SystemExit("source file(s) missing: " + ", ".join(str(p) for p in missing))
    return paths


def pairs() -> list[tuple[Path, Path]]:
    return [(src, REFERENCE_DIR / src.relative_to(REPO_ROOT)) for src in source_paths()]


def stale() -> list[tuple[Path, Path]]:
    out = []
    for src, dst in pairs():
        if not dst.is_file() or not filecmp.cmp(src, dst, shallow=False):
            out.append((src, dst))
    # Copies whose source no longer exists are stale too.
    expected = {dst for _, dst in pairs()}
    for extra in REFERENCE_DIR.rglob("*"):
        if extra.is_file() and extra not in expected:
            out.append((Path("<removed>"), extra))
    return out


def main(argv: list[str]) -> int:
    if "--check" in argv:
        bad = stale()
        for src, dst in bad:
            print(f"stale: {dst.relative_to(HERE)}  (source: {src})")
        return 1 if bad else 0
    for src, dst in pairs():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src, dst)
        print(f"synced {dst.relative_to(HERE)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
