# SPDX-FileCopyrightText: 2026 mentalfoundry
# SPDX-License-Identifier: GPL-3.0
"""Generate the PokketSX application icon in every size and format the build uses.

The icon is a pixel "P" on a coral screen in an ink tile. It is the colour sibling of the
pokketstation icon, which uses the same shapes with a pale screen and an ink "P".

All shapes sit on a 64-unit grid, and every edge of the "P" is a multiple of 4 units, so the
letter lands on whole pixels at 16 px and every larger power of two. Edit the shapes here and
run this script again; it rewrites the SVG and every raster file.

Usage: python scripts/icons/generate-icons.py   (needs Pillow)
"""

from pathlib import Path

from PIL import Image, ImageDraw

INK = "#1e2a44"
SCREEN = "#e85d3f"
LETTER = "#fff3ea"

TILE = (0, 0, 64, 64, 14)      # x0, y0, x1, y1, corner radius
SCREEN_RECT = (8, 8, 56, 56, 6)
# A 3x5 grid of 8-unit blocks: stem, top bar, right side of the bowl, middle bar.
LETTER_RECTS = [(20, 12, 28, 52), (28, 12, 44, 20), (36, 20, 44, 28), (28, 28, 44, 36)]

ROOT = Path(__file__).resolve().parents[2]
QT = ROOT / "src" / "pokketsx-qt"


def svg() -> str:
    def rect(r, fill):
        x0, y0, x1, y1 = r[:4]
        radius = f' rx="{r[4]}"' if len(r) > 4 else ""
        return f'  <rect x="{x0}" y="{y0}" width="{x1 - x0}" height="{y1 - y0}"{radius} fill="{fill}"/>\n'

    body = rect(TILE, INK) + rect(SCREEN_RECT, SCREEN) + "".join(rect(r, LETTER) for r in LETTER_RECTS)
    return f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">\n{body}</svg>\n'


def render(size: int) -> Image.Image:
    """Draw at 8x and box-filter down, so aligned edges stay sharp and curves are smoothed."""
    scale_up = 8
    canvas = size * scale_up
    unit = canvas / 64
    image = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    def box(r):
        return [round(r[0] * unit), round(r[1] * unit), round(r[2] * unit) - 1, round(r[3] * unit) - 1]

    draw.rounded_rectangle(box(TILE), radius=TILE[4] * unit, fill=INK)
    draw.rounded_rectangle(box(SCREEN_RECT), radius=SCREEN_RECT[4] * unit, fill=SCREEN)
    for r in LETTER_RECTS:
        draw.rectangle(box(r), fill=LETTER)
    return image.resize((size, size), Image.Resampling.BOX)


def save_png(path: Path, size: int) -> None:
    render(size).save(path, optimize=True)
    print(f"{path.relative_to(ROOT)}  {size}x{size}")


def main() -> None:
    (Path(__file__).parent / "pokketsx-icon.svg").write_text(svg(), encoding="utf-8", newline="\n")
    print("scripts/icons/pokketsx-icon.svg")

    ico_sizes = [16, 32, 48, 64, 128, 256]
    ico = [render(s) for s in ico_sizes]
    ico[-1].save(QT / "pokketsx-qt.ico", sizes=[(s, s) for s in ico_sizes], append_images=ico[:-1])
    print(f"src/pokketsx-qt/pokketsx-qt.ico  {ico_sizes}")

    icns_sizes = [16, 32, 64, 128, 256, 512, 1024]
    icns = [render(s) for s in icns_sizes]
    icns[-1].save(QT / "PokketSX.icns", append_images=icns[:-1])
    print(f"src/pokketsx-qt/PokketSX.icns  {icns_sizes}")

    icons = QT / "resources" / "icons"
    save_png(icons / "app-icon.png", 256)
    save_png(icons / "app-icon-128.png", 128)
    save_png(icons / "app-icon-64.png", 64)
    save_png(icons / "update-icon.png", 48)
    save_png(ROOT / "data" / "resources" / "images" / "app-icon.png", 512)
    save_png(ROOT / "scripts" / "io.github.mentalfoundry.PokketSX.png", 512)


if __name__ == "__main__":
    main()