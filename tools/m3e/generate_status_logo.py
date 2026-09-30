#!/usr/bin/env python3
"""Generate minui-compatible status/header logos from the pinned artwork."""

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ASSETS = Path(__file__).resolve().parent / "assets"
DENSITIES = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}


def generate(out: Path) -> None:
    spec = json.loads((ASSETS / "uwu-Rec-SOURCE.json").read_text(encoding="utf-8"))
    source = ASSETS / "uwu-Rec.png"
    if hashlib.sha256(source.read_bytes()).hexdigest() != spec["sha256"]:
        raise ValueError("Status logo source SHA-256 mismatch")
    with Image.open(source) as image:
        rgba = image.convert("RGBA")
    if list(rgba.size) != spec["original_size"]:
        raise ValueError("Unexpected source dimensions")
    bounds = rgba.getchannel("A").getbbox()
    if list(bounds or ()) != spec["alpha_bounds"]:
        raise ValueError("Unexpected transparent padding")
    art = rgba.crop(bounds)
    for density, scale in DENSITIES.items():
        width = round(spec["display_width_dp"] * scale)
        height = round(width * art.height / art.width)
        header_height = round(spec["header_height_dp"] * scale)
        header_width = round(header_height * art.width / art.height)
        for name, size in (("uwu_recovery_status", (width, height)),
                           ("uwu_recovery_header", (header_width, header_height))):
            resized = art.resize(size, Image.Resampling.LANCZOS)
            # minui display surfaces accept RGB; gr_blit does not alpha blend.
            background = Image.new("RGBA", size, tuple(spec["background"]) + (255,))
            rgb = Image.alpha_composite(background, resized).convert("RGB")
            target = out / f"res-{density}" / "images" / (name + ".png")
            target.parent.mkdir(parents=True, exist_ok=True)
            rgb.save(target, optimize=True)
            print(f"{target}: {size[0]}x{size[1]} RGB")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT, help="Output repository root")
    generate(parser.parse_args().out)
