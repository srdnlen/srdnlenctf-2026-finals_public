#!/usr/bin/env python3
"""Crop and upscale a region of a screenshot so on-screen text is readable.

Usage:
    tools/zoom.py SRC DST --crop X,Y,W,H [--scale N]
"""
import argparse

from PIL import Image


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--crop", help="x,y,w,h in screen points; omit for full image")
    ap.add_argument("--scale", type=int, default=2)
    args = ap.parse_args()

    image = Image.open(args.src)
    if args.crop:
        x, y, w, h = (int(v) for v in args.crop.split(","))
        image = image.crop((x, y, x + w, y + h))
    if args.scale != 1:
        image = image.resize(
            (image.width * args.scale, image.height * args.scale), Image.NEAREST
        )
    image.save(args.dst)
    print(f"{args.dst} {image.width}x{image.height}")


if __name__ == "__main__":
    main()
