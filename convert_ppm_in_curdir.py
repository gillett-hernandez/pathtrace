#!/usr/bin/env python3
import os
from PIL import Image
import time
import argparse

parser = argparse.ArgumentParser()

parser.add_argument(
    "--no-delete", action="store_true", help="doesn't delete ppm file after conversion"
)


def main(args):
    # print(args)
    for a, b, c in os.walk("."):
        for file in c:
            if file.endswith(".ppm"):
                ppmstat = os.stat(file)
                filepng = file.replace(".ppm", ".png")
                should_replace = True
                if os.path.exists(filepng):
                    # check if ppm is newer than png, only then will we overwrite
                    pngstat = os.stat(filepng)
                    if ppmstat.st_mtime <= pngstat.st_mtime:
                        should_replace = False
                # print(file, filepng)
                if should_replace:
                    try:
                        tmp_file_png = filepng.replace(".png", ".tmp.png")
                        im = Image.open(file)
                        im.save(tmp_file_png)
                        os.remove(filepng)
                        os.rename(tmp_file_png, filepng)
                        os.remove(tmp_file_png)
                        if not args.no_delete:
                            # print("deleting")
                            os.remove(file)
                    except Exception as e:
                        # print(e)
                        pass


if __name__ == "__main__":
    main(parser.parse_args())
