from io import BytesIO
from pathlib import Path
from zipfile import ZipFile

from PIL import Image, ImageEnhance


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets_online"
GRAPHICS = ROOT / "graphics"
CANVAS_SIZE = (64, 64)


def read_zip_image(archive_name: str, suffix: str) -> Image.Image:
    with ZipFile(ASSETS / archive_name) as archive:
        matching = [name for name in archive.namelist() if name.lower().endswith(suffix.lower())]
        if len(matching) != 1:
            raise RuntimeError(f"Expected one {suffix} in {archive_name}, found {matching}")
        return Image.open(BytesIO(archive.read(matching[0]))).convert("RGBA")


def fit_visible(image: Image.Image, max_width: int, max_height: int) -> Image.Image:
    bbox = image.getchannel("A").getbbox()
    if not bbox:
        raise RuntimeError("Source frame is transparent")

    image = image.crop(bbox)
    scale = min(max_width / image.width, max_height / image.height)
    size = (
        max(1, round(image.width * scale)),
        max(1, round(image.height * scale)),
    )
    return image.resize(size, Image.Resampling.NEAREST)


def paste_center(canvas: Image.Image, sprite: Image.Image, center_x: int, center_y: int) -> None:
    position = (center_x - sprite.width // 2, center_y - sprite.height // 2)
    canvas.alpha_composite(sprite, position)


def clear_flat_background(image: Image.Image) -> Image.Image:
    image = image.copy()
    background = image.getpixel((0, 0))[:3]
    pixels = image.load()

    for y in range(image.height):
        for x in range(image.width):
            red, green, blue, alpha = pixels[x, y]
            if (red, green, blue) == background:
                pixels[x, y] = (red, green, blue, 0)

    return image


def make_leopard(image: Image.Image) -> Image.Image:
    image = ImageEnhance.Color(image).enhance(0.75)
    pixels = image.load()
    width, height = image.size

    for y in range(height):
        for x in range(width):
            red, green, blue, alpha = pixels[x, y]
            if alpha >= 32 and red > 85 and green > 55 and (x * 5 + y * 7) % 19 < 3:
                pixels[x, y] = (max(24, red // 3), max(18, green // 3), max(12, blue // 3), alpha)

    return image


def save_indexed_bmp(image: Image.Image, name: str) -> None:
    rgba = image.convert("RGBA")
    rgba_pixels = list(rgba.get_flattened_data())
    opaque = [pixel[:3] for pixel in rgba_pixels if pixel[3] >= 32]
    if not opaque:
        raise RuntimeError(f"{name} has no opaque pixels")

    sample = Image.new("RGB", (len(opaque), 1))
    sample.putdata(opaque)
    color_count = min(15, len(set(opaque)))
    quantized = sample.quantize(colors=color_count, method=Image.Quantize.MEDIANCUT)
    source_palette = quantized.getpalette()
    used_indices = sorted(index for _, index in quantized.getcolors() or [])
    colors = [tuple(source_palette[index * 3:index * 3 + 3]) for index in used_indices]

    output = Image.new("P", CANVAS_SIZE, 0)
    palette = [255, 0, 255]
    for color in colors:
        palette.extend(color)
    palette.extend([0] * (768 - len(palette)))
    output.putpalette(palette)

    indexed_pixels = []
    for red, green, blue, alpha in rgba_pixels:
        if alpha < 32:
            indexed_pixels.append(0)
            continue

        nearest = min(
            range(len(colors)),
            key=lambda index: (
                (red - colors[index][0]) ** 2
                + (green - colors[index][1]) ** 2
                + (blue - colors[index][2]) ** 2
            ),
        )
        indexed_pixels.append(nearest + 1)

    output.putdata(indexed_pixels)
    destination = GRAPHICS / f"{name}.bmp"
    output.save(destination, format="BMP")
    print(f"wrote {destination.relative_to(ROOT)} ({output.width}x{output.height}, indexed)")


def build_beasts() -> Image.Image:
    lion_sheet = read_zip_image("lpc_animals_2022_v1.1.zip", "/lion.png")
    lioness_sheet = read_zip_image("lpc_animals_2022_v1.1.zip", "/lioness.png")
    wolf_sheet = clear_flat_background(Image.open(ASSETS / "wolf_sit.png").convert("RGBA"))

    lion = fit_visible(lion_sheet.crop((0, 64, 64, 128)), 30, 23)
    leopard = fit_visible(lioness_sheet.crop((0, 128, 64, 192)), 30, 23)
    leopard = make_leopard(leopard)
    wolf = fit_visible(wolf_sheet.crop((0, 0, 66, 66)), 35, 24)

    canvas = Image.new("RGBA", CANVAS_SIZE, (0, 0, 0, 0))
    paste_center(canvas, lion, 17, 15)
    paste_center(canvas, leopard, 47, 34)
    paste_center(canvas, wolf, 19, 50)
    return canvas


def build_charon() -> Image.Image:
    boat_sheet = read_zip_image("ferryman.zip", "boat-idle.png")
    boat = fit_visible(boat_sheet.crop((0, 0, 64, 80)), 56, 58)
    canvas = Image.new("RGBA", CANVAS_SIZE, (0, 0, 0, 0))
    paste_center(canvas, boat, 32, 32)
    return canvas


def build_gate() -> Image.Image:
    gate_sheet = Image.open(ASSETS / "door.png").convert("RGBA")
    gate = fit_visible(gate_sheet.crop((0, 0, 64, 64)), 56, 58)
    canvas = Image.new("RGBA", CANVAS_SIZE, (0, 0, 0, 0))
    paste_center(canvas, gate, 32, 32)
    return canvas


def build_flame() -> Image.Image:
    fire_sheet = Image.open(ASSETS / "fire1_64.png").convert("RGBA")
    flame = fit_visible(fire_sheet.crop((256, 0, 320, 64)), 44, 58)
    canvas = Image.new("RGBA", CANVAS_SIZE, (0, 0, 0, 0))
    paste_center(canvas, flame, 32, 32)
    return canvas


def main() -> None:
    GRAPHICS.mkdir(exist_ok=True)
    save_indexed_bmp(build_beasts(), "beasts")
    save_indexed_bmp(build_charon(), "charon")
    save_indexed_bmp(build_gate(), "dis_gate")
    save_indexed_bmp(build_flame(), "ulysses_flame")


if __name__ == "__main__":
    main()
