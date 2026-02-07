#!/usr/bin/env python3
"""Process downloaded source images into consistent pixel-art PNG cards.

Usage:
  python tools/process_assets.py
  python tools/process_assets.py --height 256 --pixel-height 64 --quantize 24
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover
    print("Pillow is required. Install with: pip install pillow", file=sys.stderr)
    raise SystemExit(1) from exc


ROOT = pathlib.Path(__file__).resolve().parents[1]
CARDS_PATH = ROOT / "assets" / "manifest" / "cards.json"
SOURCE_DIR = ROOT / "assets" / "source"
PROCESSED_DIR = ROOT / "assets" / "processed"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Convert source images into pixel-art PNG portraits.")
    parser.add_argument("--height", type=int, default=256, help="Final card face texture height in pixels.")
    parser.add_argument(
        "--pixel-height",
        type=int,
        default=64,
        help="Intermediate downscale height used for blocky pixel look.",
    )
    parser.add_argument(
        "--quantize",
        type=int,
        default=28,
        help="Palette color count used before final upscale.",
    )
    parser.add_argument(
        "--contact-sheet",
        action="store_true",
        help="Create assets/processed/contact_sheet.png preview grid.",
    )
    return parser.parse_args()


def load_cards(path: pathlib.Path) -> list[dict]:
    if not path.exists():
        raise FileNotFoundError(f"Missing cards manifest: {path}")
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    cards = data.get("cards", [])
    if not isinstance(cards, list):
        raise ValueError("cards.json format is invalid: expected key 'cards' as list.")
    return cards


def find_source_image(slug: str) -> pathlib.Path | None:
    for ext in (".png", ".jpg", ".jpeg", ".webp"):
        candidate = SOURCE_DIR / f"{slug}{ext}"
        if candidate.exists():
            return candidate
    return None


def center_crop_to_ratio(image: Image.Image, target_ratio: float) -> Image.Image:
    width, height = image.size
    current_ratio = width / height
    if current_ratio > target_ratio:
        new_width = int(height * target_ratio)
        x0 = (width - new_width) // 2
        return image.crop((x0, 0, x0 + new_width, height))
    new_height = int(width / target_ratio)
    y0 = (height - new_height) // 2
    return image.crop((0, y0, width, y0 + new_height))


def process_one(
    source: pathlib.Path,
    destination: pathlib.Path,
    final_width: int,
    final_height: int,
    pixel_width: int,
    pixel_height: int,
    quantize: int,
) -> None:
    with Image.open(source) as image:
        image = image.convert("RGB")
        image = center_crop_to_ratio(image, target_ratio=3.0 / 4.0)
        image = image.resize((pixel_width, pixel_height), Image.Resampling.NEAREST)
        image = image.quantize(colors=max(2, quantize), method=Image.Quantize.MEDIANCUT).convert("RGB")
        image = image.resize((final_width, final_height), Image.Resampling.NEAREST)
        image.save(destination, format="PNG")


def create_contact_sheet(outputs: list[pathlib.Path], destination: pathlib.Path, tile_width: int, tile_height: int) -> None:
    if not outputs:
        return
    columns = 4
    rows = (len(outputs) + columns - 1) // columns
    margin = 12
    sheet = Image.new(
        "RGB",
        (columns * tile_width + (columns + 1) * margin, rows * tile_height + (rows + 1) * margin),
        color=(20, 20, 26),
    )

    for index, output in enumerate(outputs):
        with Image.open(output) as tile:
            row = index // columns
            column = index % columns
            x = margin + column * (tile_width + margin)
            y = margin + row * (tile_height + margin)
            sheet.paste(tile, (x, y))

    sheet.save(destination, format="PNG")


def main() -> int:
    args = parse_args()
    PROCESSED_DIR.mkdir(parents=True, exist_ok=True)

    try:
        cards = load_cards(CARDS_PATH)
    except Exception as exc:  # noqa: BLE001
        print(f"Failed to load cards manifest: {exc}", file=sys.stderr)
        return 1

    if args.pixel_height <= 0 or args.height <= 0:
        print("--height and --pixel-height must be > 0", file=sys.stderr)
        return 1

    final_height = args.height
    final_width = max(1, int(round(final_height * (3.0 / 4.0))))
    pixel_height = args.pixel_height
    pixel_width = max(1, int(round(pixel_height * (3.0 / 4.0))))

    processed_files: list[pathlib.Path] = []
    failures = 0

    for card in cards:
        slug = str(card.get("slug", "")).strip()
        name = str(card.get("name", slug)).strip()
        if not slug:
            print(f"Skipping malformed manifest entry: {card!r}", file=sys.stderr)
            failures += 1
            continue

        source = find_source_image(slug)
        if source is None:
            print(f"Missing source image for {slug} in {SOURCE_DIR}", file=sys.stderr)
            failures += 1
            continue

        destination = PROCESSED_DIR / f"{slug}.png"
        try:
            process_one(
                source,
                destination,
                final_width=final_width,
                final_height=final_height,
                pixel_width=pixel_width,
                pixel_height=pixel_height,
                quantize=args.quantize,
            )
            print(f"Processed {name} -> {destination}")
            processed_files.append(destination)
        except OSError as exc:
            print(f"Failed to process {slug}: {exc}", file=sys.stderr)
            failures += 1

    if args.contact_sheet:
        contact_sheet_path = PROCESSED_DIR / "contact_sheet.png"
        create_contact_sheet(
            processed_files,
            contact_sheet_path,
            tile_width=final_width,
            tile_height=final_height,
        )
        print(f"Generated contact sheet -> {contact_sheet_path}")

    print(f"\nDone. processed={len(processed_files)} failed={failures}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
