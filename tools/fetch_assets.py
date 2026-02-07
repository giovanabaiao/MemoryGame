#!/usr/bin/env python3
"""Download source character images listed in assets/manifest/sources.json.

Usage:
  python tools/fetch_assets.py
  python tools/fetch_assets.py --force
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
import urllib.error
import urllib.parse
import urllib.request


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCES_PATH = ROOT / "assets" / "manifest" / "sources.json"
SOURCE_DIR = ROOT / "assets" / "source"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Download raw source images for all cards.")
    parser.add_argument("--force", action="store_true", help="Overwrite files if they already exist.")
    parser.add_argument("--timeout", type=float, default=20.0, help="HTTP timeout in seconds.")
    return parser.parse_args()


def load_manifest(path: pathlib.Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"Missing manifest: {path}")
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def output_name(slug: str, url: str) -> pathlib.Path:
    parsed = urllib.parse.urlparse(url)
    ext = pathlib.Path(parsed.path).suffix.lower()
    if ext not in {".jpg", ".jpeg", ".png", ".webp"}:
        ext = ".jpg"
    return SOURCE_DIR / f"{slug}{ext}"


def download_file(url: str, destination: pathlib.Path, timeout: float) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": "MemoryGameAssetFetcher/1.0"})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        content_type = response.headers.get("Content-Type", "")
        if "image" not in content_type:
            raise RuntimeError(f"URL does not look like an image (Content-Type: {content_type})")
        data = response.read()
    destination.write_bytes(data)


def main() -> int:
    args = parse_args()
    SOURCE_DIR.mkdir(parents=True, exist_ok=True)

    try:
        manifest = load_manifest(SOURCES_PATH)
    except Exception as exc:  # noqa: BLE001
        print(f"Error loading manifest: {exc}", file=sys.stderr)
        return 1

    cards = manifest.get("cards", [])
    if not isinstance(cards, list) or not cards:
        print("No cards found in sources manifest.", file=sys.stderr)
        return 1

    success = 0
    skipped = 0
    failed = 0

    for entry in cards:
        name = str(entry.get("name", "")).strip()
        slug = str(entry.get("slug", "")).strip()
        url = str(entry.get("url", "")).strip()

        if not slug:
            print(f"Skipping entry without slug: {entry!r}")
            skipped += 1
            continue

        if not url:
            print(f"Skipping {slug}: missing URL in sources manifest.")
            skipped += 1
            continue

        destination = output_name(slug, url)

        if destination.exists() and not args.force:
            print(f"Skipping existing file: {destination}")
            skipped += 1
            continue

        try:
            download_file(url, destination, timeout=args.timeout)
            print(f"Downloaded {name or slug} -> {destination}")
            success += 1
        except (urllib.error.URLError, TimeoutError, RuntimeError, OSError) as exc:
            print(f"Failed {slug}: {exc}", file=sys.stderr)
            failed += 1

    print(
        f"\nDone. success={success} skipped={skipped} failed={failed} "
        f"(source dir: {SOURCE_DIR})"
    )
    return 1 if failed > 0 else 0


if __name__ == "__main__":
    raise SystemExit(main())
