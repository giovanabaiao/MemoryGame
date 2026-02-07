#!/usr/bin/env python3
"""Populate assets/manifest/sources.json with Star Wars character image URLs.

Primary source: Wookieepedia MediaWiki API (pageimages original image URL).
One explicit override is used for Darth Vader to avoid duplicate art with Anakin.
"""

from __future__ import annotations

import json
import pathlib
import urllib.parse
import urllib.request


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCES_PATH = ROOT / "assets" / "manifest" / "sources.json"

API_BASE = "https://starwars.fandom.com/api.php"
USER_AGENT = "MemoryGameAssetBot/1.0 (local project setup)"

# Maps manifest slug -> Wookieepedia title
TITLE_BY_SLUG = {
    "luke_skywalker": "Luke Skywalker",
    "leia_organa": "Leia Organa",
    "darth_vader": "Darth Vader",
    "anakin_skywalker": "Anakin Skywalker",
    "obi_wan_kenobi": "Obi-Wan Kenobi",
    "yoda": "Yoda",
    "han_solo": "Han Solo",
    "chewbacca": "Chewbacca",
    "emperor_palpatine": "Palpatine",
    "rey": "Rey Skywalker",
    "kylo_ren": "Kylo Ren",
    "r2_d2": "R2-D2",
    "c_3po": "C-3PO",
    "lando_calrissian": "Lando Calrissian",
    "mandalorian": "Din Djarin",
    "padme_amidala": "Padmé Amidala",
}

# Keep Vader visually distinct from Anakin in the card set.
OVERRIDE_URL_BY_SLUG = {
    "darth_vader": "https://upload.wikimedia.org/wikipedia/commons/9/9c/Darth_Vader_-_2007_Disney_Weekends.jpg",
}


def request_json(url: str) -> dict:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=20) as response:
        return json.loads(response.read().decode("utf-8"))


def canonicalize_image_url(url: str) -> str:
    """Strip Fandom revision suffix so the direct file path keeps its extension."""
    marker = "/revision/"
    if marker in url:
        return url.split(marker, maxsplit=1)[0]
    return url


def fetch_image_url(title: str) -> str:
    params = {
        "action": "query",
        "format": "json",
        "prop": "pageimages",
        "piprop": "original",
        "titles": title,
        "redirects": "1",
    }
    url = API_BASE + "?" + urllib.parse.urlencode(params)
    payload = request_json(url)
    pages = payload.get("query", {}).get("pages", {})
    for page in pages.values():
        source = page.get("original", {}).get("source")
        if source:
            return canonicalize_image_url(source)
    raise RuntimeError(f"No page image found for title '{title}'.")


def main() -> int:
    with SOURCES_PATH.open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    cards = manifest.get("cards", [])
    if not isinstance(cards, list):
        raise RuntimeError("Invalid sources manifest format: expected cards list.")

    failures = 0
    for card in cards:
        slug = str(card.get("slug", "")).strip()
        if not slug:
            failures += 1
            continue

        if slug in OVERRIDE_URL_BY_SLUG:
            card["url"] = OVERRIDE_URL_BY_SLUG[slug]
            print(f"set {slug}: override URL")
            continue

        title = TITLE_BY_SLUG.get(slug)
        if not title:
            print(f"missing title mapping for slug: {slug}")
            card["url"] = ""
            failures += 1
            continue

        try:
            card["url"] = fetch_image_url(title)
            print(f"set {slug}: {card['url']}")
        except Exception as exc:  # noqa: BLE001
            print(f"failed {slug}: {exc}")
            card["url"] = ""
            failures += 1

    with SOURCES_PATH.open("w", encoding="utf-8") as handle:
        json.dump(manifest, handle, indent=2, ensure_ascii=False)
        handle.write("\n")

    print(f"\nUpdated: {SOURCES_PATH}")
    if failures:
        print(f"Completed with {failures} failures.")
        return 1
    print("Completed successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
