#!/usr/bin/env python3
"""Preview status PNGs with ScreenRecoveryUI's layout; not a device screenshot."""

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
STATES = (
    ("Installing update", "installing_text", True),
    ("Security update", "installing_security_text", True),
    ("Erasing", "erasing_text", False),
    ("Error", "error_text", False),
    ("No command", "no_command_text", False),
)


def localized(assets: Path, name: str, locale: str) -> Image.Image:
    """Decode the metadata rows used by res_create_localized_alpha_surface."""
    with Image.open(assets / (name + ".png")) as image:
        atlas = image.convert("L")
    y = 0
    while y < atlas.height:
        row = atlas.crop((0, y, atlas.width, y + 1)).tobytes()
        width = row[0] + 256 * row[1]
        height = row[2] + 256 * row[3]
        lang = row[5:].split(b"\0", 1)[0].decode("ascii")
        if not (0 < width <= atlas.width and 0 < height < atlas.height - y):
            raise ValueError(f"Invalid localized PNG: {name} at row {y}")
        if lang == locale or lang == locale.split("-")[0]:
            return atlas.crop((0, y + 1, width, y + 1 + height))
        y += height + 1
    raise ValueError(f"Missing locale {locale} in {name}")


def render(out: Path, locale: str) -> None:
    # diting portrait sample, matching the stock xxhdpi resource set.
    width, height, density = 1220, 2712, 3
    assets = ROOT / "res-xxhdpi" / "images"
    with Image.open(assets / "uwu_recovery_status.png") as image:
        logo = image.copy()
    with Image.open(assets / "progress_empty.png") as image:
        empty = image.convert("RGB")
    with Image.open(assets / "progress_fill.png") as image:
        fill = image.convert("RGB")
    panel_width = 260
    panel_height = round(panel_width * height / width)
    gap = 20
    sheet = Image.new("RGB", (gap + len(STATES) * (panel_width + gap), panel_height + 130),
                      (241, 238, 248))
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default(size=20)
    label_font = ImageFont.load_default(size=16)
    draw.text((gap, 16), "M3E Recovery | uwu status artwork", font=font, fill=(39, 32, 52))
    draw.text((gap, 48), "Host preview from source resources; sample progress, not a device screenshot.",
              font=label_font, fill=(87, 78, 104))
    out.parent.mkdir(parents=True, exist_ok=True)
    for index, (title, name, progress) in enumerate(STATES):
        text = localized(assets, name, locale)
        installing_name = name if name == "installing_security_text" else "installing_text"
        installing = localized(assets, installing_name, locale)
        # Same integer layout as GetProgressBaseline / GetTextBaseline / GetAnimationBaseline.
        elements = logo.height + 68 * density + installing.height + 32 * density + fill.height
        progress_y = height - (height - elements) // 2 - fill.height
        text_y = progress_y - 32 * density - installing.height
        logo_y = text_y - 68 * density - logo.height
        canvas = Image.new("RGB", (width, height), "black")
        canvas.paste(logo, ((width - logo.width) // 2, logo_y))
        canvas.paste((255, 255, 255), ((width - text.width) // 2, text_y), text)
        if progress:
            bar_x = (width - empty.width) // 2
            canvas.paste(empty, (bar_x, progress_y))
            canvas.paste(fill.crop((0, 0, int(fill.width * 0.43), fill.height)), (bar_x, progress_y))
        x = gap + index * (panel_width + gap)
        draw.text((x, 86), title, font=label_font, fill=(39, 32, 52))
        sheet.paste(canvas.resize((panel_width, panel_height), Image.Resampling.LANCZOS), (x, 112))
    sheet.save(out)
    print(out)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT / "tools/m3e/preview/status-preview.png")
    parser.add_argument("--locale", default="zh-CN", choices=["zh-CN", "en-US"])
    args = parser.parse_args()
    render(args.out, args.locale)
