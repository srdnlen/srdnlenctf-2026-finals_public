from __future__ import annotations

import io
import os
from pathlib import Path

from PIL import Image

UPLOAD_DIR = Path(os.environ.get("UPLOAD_DIR", str(Path(__file__).resolve().parent / "uploads")))

MAX_PHOTO_BYTES = 3 * 1024 * 1024
MAX_SIDE = 512
ALLOWED_FORMATS = {"PNG", "JPEG", "WEBP"}

Image.MAX_IMAGE_PIXELS = 24_000_000

class PhotoError(Exception):
    pass

def photo_path(uid: int) -> Path:
    return UPLOAD_DIR / f"{uid}.png"

def has_photo(uid: int) -> bool:
    return photo_path(uid).is_file()

def delete_photo(uid: int) -> None:
    try:
        photo_path(uid).unlink()
    except FileNotFoundError:
        pass

def save_profile_photo(uid: int, file_storage) -> str:
    raw = file_storage.read(MAX_PHOTO_BYTES + 1)
    if not raw:
        raise PhotoError("No image uploaded")
    if len(raw) > MAX_PHOTO_BYTES:
        raise PhotoError("Image too large (max 3 MB)")

    try:
        probe = Image.open(io.BytesIO(raw))
        fmt = probe.format
        probe.verify()
    except PhotoError:
        raise
    except Exception:
        raise PhotoError("Not a valid image file")

    if fmt not in ALLOWED_FORMATS:
        raise PhotoError("Only PNG, JPEG or WEBP images are allowed")

    try:
        im = Image.open(io.BytesIO(raw))
        im = im.convert("RGB")
        im.thumbnail((MAX_SIDE, MAX_SIDE))
        UPLOAD_DIR.mkdir(parents=True, exist_ok=True)
        out = photo_path(uid)
        tmp = UPLOAD_DIR / f"{uid}.tmp.png"
        im.save(tmp, format="PNG", optimize=True)
        os.replace(tmp, out)
    except PhotoError:
        raise
    except Exception:
        raise PhotoError("Could not process image")

    return out.name
