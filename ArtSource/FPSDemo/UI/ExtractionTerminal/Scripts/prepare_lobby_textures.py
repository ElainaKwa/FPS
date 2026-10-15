"""Prepare code-drawn UI shading and a thumbnail of our existing map capture."""
from pathlib import Path
from PIL import Image
import math

ROOT = Path(__file__).resolve().parents[5]
OUT = Path(__file__).resolve().parents[1] / 'Textures'
OUT.mkdir(parents=True, exist_ok=True)
w, h = 800, 450
image = Image.new('RGBA', (w, h))
pixels = []
for y in range(h):
    for x in range(w):
        edge = abs(x / (w - 1) - .5) * 2
        top = max(0, 1 - y / 135) ** 2 * .30
        bottom = max(0, (y - 280) / 169) ** 1.5 * .65
        alpha = min(.82, edge ** 3 * .60 + top + bottom)
        pixels.append((3, 7, 9, round(alpha * 255)))
image.putdata(pixels)
image.save(OUT / 'T_LobbyVignette.png')
source = ROOT / 'ArtSource/FPSDemo/Environment/CrossingKit/Previews/UnrealMapOverview.png'
with Image.open(source) as image:
    # The original editor/map capture stays intact; the menu uses a small derivative.
    ratio = 372 / 154
    height = min(image.height, round(image.width / ratio))
    top = max(0, (image.height - height) // 2)
    image.crop((0, top, image.width, top + height)).resize((744, 308), Image.Resampling.LANCZOS).convert('RGB').save(OUT / 'T_LobbyIndustrialCrossing.png')
print('LOBBY_TEXTURES prepared', OUT)
