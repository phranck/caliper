#!/usr/bin/env python3
"""Finds a doc comment that documents nothing.

A doc comment describes whatever declaration follows it in the file. Where a
closing `*/` is immediately followed by an opening `/**`, with no declaration
between the two, the first one has nothing left to describe: something that
used to stand between them was moved or removed, and the comment was not
moved with it.

Run over every tracked header and source, so a vendored dependency such as
`test/component/managed_components`, never checked into this repository, is
never read.

    python3 tools/check_doc_comments.py

Exits with 1 and a line per finding where it finds one, 0 otherwise.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

#: Which tracked files this reads. Anything else in the tree, generated
#: output and the vendored library among them, is not source this project
#: hand-writes comments into.
SOURCE_DIRECTORIES = ("include", "src")
SOURCE_SUFFIXES = (".h", ".cc")


def tracked_sources() -> list[Path]:
    """Lists the tracked headers and sources this check reads.

    Uses `git ls-files` rather than a filesystem walk, so a directory git
    ignores, `managed_components` among them, is never listed and therefore
    never opened.

    @returns Their paths, relative to the repository root.
    """
    listed = subprocess.run(
        ["git", "ls-files", *SOURCE_DIRECTORIES],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.splitlines()
    return [Path(path) for path in listed if path.endswith(SOURCE_SUFFIXES)]


def orphaned_comments(path: Path) -> list[int]:
    """Finds every doc comment in one file that documents nothing.

    @param path The file to read.
    @returns The line number of each opening `/**` that directly follows
             another comment's closing `*/`, one-based as an editor counts.
    """
    lines = path.read_text(encoding="utf-8").splitlines()
    return [
        index + 2
        for index in range(len(lines) - 1)
        if lines[index].strip() == "*/" and lines[index + 1].strip() == "/**"
    ]


def main() -> int:
    findings = 0
    for path in tracked_sources():
        for line in orphaned_comments(path):
            print(f"{path}:{line}: this doc comment immediately follows another's closing `*/`, "
                  f"with no declaration between them, so the one above it documents nothing")
            findings += 1
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
