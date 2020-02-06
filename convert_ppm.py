#!/usr/bin/env python3
import os
from PIL import Image
import time
import argparse

parser = argparse.ArgumentParser()

parser.add_argument(
    "--no-delete", action="store_true", help="doesn't delete ppm file after conversion"
)

parser.add_argument("-p", "--path", type=str, default=".", help="path to convert")


def main(args):
    # print(args)
    for a, b, c in os.walk(args.path):
        for file in c:
            if file.endswith(".ppm"):
                filepath = os.path.join(a, file)
                ppmstat = os.stat(filepath)
                filepng = filepath.replace(".ppm", ".png")
                should_replace = True
                if os.path.exists(filepng):
                    # check if ppm is newer than png, only then will we overwrite
                    pngstat = os.stat(filepng)
                    # print(ppmstat.st_mtime - pngstat.st_mtime)
                    if ppmstat.st_mtime <= pngstat.st_mtime:
                        should_replace = False
                # print(file, filepng)
                if should_replace:
                    try:
                        tmp_file_png = filepng.replace(".png", ".tmp.png")
                        im = Image.open(filepath)
                        im.save(tmp_file_png)
                        print("doing stuff0")
                        try:
                            os.remove(filepng)
                        except FileNotFoundError:
                            pass
                        print("doing stuff1")
                        os.rename(tmp_file_png, filepng)
                        print("doing stuff2")
                        if not args.no_delete:
                            # print("deleting")
                            os.remove(filepath)
                    except Exception as e:
                        print(e)
                        pass


if __name__ == "__main__":
    main(parser.parse_args())
