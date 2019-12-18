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
    for a, b, c in os.walk("."):
        for file in c:
            if file.endswith(".ppm"):
                try:
                    im = Image.open(file)
                    im.save(file.replace(".ppm", ".png"))
                    if not args.no_delete:
                        os.remove(file)
                except:
                    pass


if __name__ == "__main__":
    main(parser.parse_args())
