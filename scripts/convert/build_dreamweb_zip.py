#!/usr/bin/env python3
"""Package a DreamWeb DOS directory for the openfpgaOS ScummVM port."""

import argparse
from pathlib import Path
from zipfile import ZIP_STORED, ZipFile


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="directory containing DREAMWEB.*")
    parser.add_argument("output", type=Path, help="destination ZIP")
    args = parser.parse_args()

    excluded_roots = {".git", ".agents", ".codex", "openfpgaOS-ScummVM"}
    files = sorted(
        p for p in args.source.rglob("*")
        if p.is_file() and not any(part in excluded_roots for part in p.relative_to(args.source).parts)
    )
    if not files:
        raise SystemExit("No DREAMWEB.* files found")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(args.output, "w", compression=ZIP_STORED) as archive:
        for path in files:
            archive.write(path, arcname=path.relative_to(args.source))

    print(f"Packaged {len(files)} files -> {args.output}")


if __name__ == "__main__":
    main()
