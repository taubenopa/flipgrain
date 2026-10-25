"""Erzeugt die Wizard-Bilder fuer den Inno-Setup-Installer (dunkles Neon-Design).
Aufruf:  python installer/make_assets.py   (benoetigt Pillow)"""
import math, os, random
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

OUT = os.path.join(os.path.dirname(__file__), "assets")
os.makedirs(OUT, exist_ok=True)
CYAN, VIOLET, MAGENTA = (61, 242, 255), (138, 92, 255), (255, 61, 203)

def lerp(a, b, t): return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))

def font(size, bold=True):
    for p in ("C:/Windows/Fonts/segoeuib.ttf", "C:/Windows/Fonts/arialbd.ttf",
              "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"):
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()

def background(w, h):
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        c = lerp((14, 17, 30), (5, 6, 11), y / h)
        for x in range(w):
            px[x, y] = c
    glow = Image.new("RGB", (w, h), (0, 0, 0))
    d = ImageDraw.Draw(glow)
    d.ellipse([-w * 0.6, -h * 0.15, w * 1.0, h * 0.35], fill=(20, 90, 110))
    d.ellipse([w * 0.1, h * 0.65, w * 1.8, h * 1.2], fill=(90, 20, 80))
    glow = glow.filter(ImageFilter.GaussianBlur(w * 0.18))
    return ImageChops.add(img, glow, scale=1.6)

def bars(img, y0, y1, seed=7):
    random.seed(seed)
    w = img.width
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    n = 14
    bw = w / n
    mid = (y0 + y1) / 2
    for i in range(n):
        env = 0.35 + 0.65 * abs(math.sin(i * 0.55 + 0.6))
        h = (y1 - y0) * 0.5 * env * random.uniform(0.6, 1.0)
        c = lerp(CYAN, MAGENTA, i / (n - 1))
        x0 = i * bw + bw * 0.22
        d.rounded_rectangle([x0, mid - h, x0 + bw * 0.56, mid + h], radius=bw * 0.2, fill=c + (230,))
    glow = layer.filter(ImageFilter.GaussianBlur(w * 0.04))
    img.paste(glow, (0, 0), glow)
    img.paste(layer, (0, 0), layer)

def large(w, h):
    img = background(w, h)
    s = w / 164.0
    bars(img, h * 0.30, h * 0.62)
    d = ImageDraw.Draw(img)
    d.text((w * 0.5, h * 0.12), "FLIP", font=font(int(26 * s)), fill=(234, 240, 255), anchor="mm")
    d.text((w * 0.5, h * 0.20), "GRAIN", font=font(int(26 * s)), fill=lerp(CYAN, MAGENTA, 0.6), anchor="mm")
    d.text((w * 0.5, h * 0.93), "SAMPLE FLIPPER", font=font(int(8 * s)), fill=(124, 136, 170), anchor="mm")
    d.line([(w * 0.2, h * 0.88), (w * 0.8, h * 0.88)], fill=(58, 68, 102), width=max(1, int(s)))
    return img

def small(w, h):
    img = background(w, h)
    bars(img, h * 0.18, h * 0.82, seed=3)
    return img

for w, h, tag in ((164, 314, ""), (246, 471, "_150"), (328, 628, "_200")):
    large(w, h).save(os.path.join(OUT, f"wizard_large{tag}.bmp"))
for w, h, tag in ((55, 55, ""), (82, 82, "_150"), (110, 110, "_200")):
    small(w, h).save(os.path.join(OUT, f"wizard_small{tag}.bmp"))
print("assets written to", OUT)
