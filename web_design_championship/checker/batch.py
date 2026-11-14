import fontforge
import sys
import os
from multiprocessing import Pool, cpu_count

def convert(svg_path):
    font = fontforge.open(svg_path)

    name = os.path.splitext(svg_path)[0]
    output = name + ".woff"

    font.generate(output)
    font.close()

    return output

if __name__ == "__main__":
    files = sys.argv[1:]

    with Pool(processes=cpu_count()) as pool:
        results = pool.map(convert, files)

    for r in results:
        print("Generated:", r)